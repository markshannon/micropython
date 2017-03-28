/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "i2c_api.h"

extern "C" {

#include "py/runtime.h"
#include "modmicrobit.h"
#include "microbitobj.h"

// Default i2c pins
#define MICROBIT_PIN_SDA P0_30
#define MICROBIT_PIN_SCL P0_0


typedef struct _microbit_i2c_obj_t {
    mp_obj_base_t base;
    i2c_t *i2c;
} microbit_i2c_obj_t;

void microbit_i2c_init(void) {
    i2c_init(microbit_i2c_obj.i2c, MICROBIT_PIN_SDA, MICROBIT_PIN_SCL);
    //i2c_frequency(microbit_i2c_obj.i2c, 100000);
}

STATIC mp_obj_t microbit_i2c_init_func(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_sda, MP_ARG_OBJ, {.u_obj = mp_const_none } },
        { MP_QSTR_scl, MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };

    // parse args
    microbit_i2c_obj_t *self = (microbit_i2c_obj_t*)pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    PinName p_sda = MICROBIT_PIN_SDA;
    PinName p_scl = MICROBIT_PIN_SCL;

    if (args[1].u_obj != mp_const_none) {
        p_sda = microbit_obj_get_pin_name(args[1].u_obj);
    }
    if (args[2].u_obj != mp_const_none) {
        p_scl = microbit_obj_get_pin_name(args[2].u_obj);
    }

    i2c_init(self->i2c, p_sda, p_scl);
    i2c_frequency(self->i2c, args[0].u_int);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_init_obj, 1, microbit_i2c_init_func);

int microbit_i2c_read(const microbit_i2c_obj_t *self, uint32_t address, char* data, int len, bool repeated) {
    int result = i2c_read(self->i2c, address, data, len, 0);
    if (!repeated) {
        i2c_stop(self->i2c);
    }
    return result;
}

STATIC mp_obj_t microbit_i2c_read_func(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_n,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_repeat,   MP_ARG_BOOL, {.u_bool = false} },
    };

    // parse args
    microbit_i2c_obj_t *self = (microbit_i2c_obj_t*)pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // do the I2C read
    vstr_t vstr;
    vstr_init_len(&vstr, args[1].u_int);
    int stop = args[2].u_bool ? 0 : 1;
    int err = microbit_i2c_read(self,  args[0].u_int << 1, vstr.buf, vstr.len, args[2].u_bool);
    if (err < vstr.len) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C read failed with error code %d", err));
    }
    // return bytes object with read data
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_read_obj, 1, microbit_i2c_read_func);

STATIC mp_obj_t microbit_i2c_write_func(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buf,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_repeat,   MP_ARG_BOOL, {.u_bool = false} },
    };

    // parse args
    microbit_i2c_obj_t *self = (microbit_i2c_obj_t*)pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // do the I2C write
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1].u_obj, &bufinfo, MP_BUFFER_READ);

    int err = microbit_i2c_write(self, args[0].u_int << 1, (const char*)bufinfo.buf, bufinfo.len, args[2].u_bool);
    if (err < (int)bufinfo.len) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C write failed with error code %d", err));
    }
    return mp_const_none;

}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_i2c_write_obj, 1, microbit_i2c_write_func);

int microbit_i2c_write(const microbit_i2c_obj_t *self, uint32_t address, const char* data, int len, bool repeated) {
    int stop = repeated ? 0 : 1;
    return i2c_write(self->i2c, address, data, len, stop);
}

STATIC const mp_map_elem_t microbit_i2c_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&microbit_i2c_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&microbit_i2c_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&microbit_i2c_write_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_i2c_locals_dict, microbit_i2c_locals_dict_table);

const mp_obj_type_t microbit_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitI2C,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_i2c_locals_dict,
};

static i2c_t i2c;

const microbit_i2c_obj_t microbit_i2c_obj = {
    {&microbit_i2c_type},
    .i2c = &i2c
};

}
