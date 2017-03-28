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

#include "nrf_gpio.h"

extern "C" {

#include "py/runtime.h"
#include "modmicrobit.h"
#include "microbitobj.h"

struct Sample
{
    int16_t x;
    int16_t y;
    int16_t z;
};

/*
enum Gesture
{
    GESTURE_NONE,
    GESTURE_UP,
    GESTURE_DOWN,
    GESTURE_LEFT,
    GESTURE_RIGHT,
    GESTURE_UPSIDE_DOWN,
    GESTURE_SHAKE
};
*/

#define MMA8653_DEFAULT_ADDR    0x3A

#define MICROBIT_PIN_ACCEL_DATA_READY 28

/*
 * MMA8653 Register map (partial)
 */
#define MMA8653_STATUS          0x00
#define MMA8653_OUT_X_MSB       0x01
#define MMA8653_WHOAMI          0x0D
#define MMA8653_XYZ_DATA_CFG    0x0E
#define MMA8653_CTRL_REG1       0x2A
#define MMA8653_CTRL_REG2       0x2B
#define MMA8653_CTRL_REG3       0x2C
#define MMA8653_CTRL_REG4       0x2D
#define MMA8653_CTRL_REG5       0x2E


typedef struct  _microbit_accelerometer_data_t {
    Sample sample;
    uint8_t sigma; // the number of ticks that the instantaneous gesture has been stable.
    bool running;
} microbit_accelerometer_data_t;

typedef struct _microbit_accelerometer_obj_t {
    mp_obj_base_t base;
    microbit_accelerometer_data_t *data;
} microbit_accelerometer_obj_t;

#define GESTURE_LIST_SIZE (8)

// We store this state globally instead of in a microbit_accelerometer_obj_t
// struct so that that whole struct does not need to go in RAM.
volatile uint16_t gesture_state = 0;                    // 1 bit per gesture
volatile uint8_t gesture_list_cur = 0;                  // index into gesture_list
volatile uint8_t gesture_list[GESTURE_LIST_SIZE] = {0}; // list of pending gestures, 4-bits per element



static int read_command(uint8_t reg, uint8_t* buffer, int length)
{
    int err = microbit_i2c_write(&microbit_i2c_obj, MMA8653_DEFAULT_ADDR, (const char *)&reg, 1, true);
    if (err != 1)
        return -1;
    return microbit_i2c_read(&microbit_i2c_obj, MMA8653_DEFAULT_ADDR, (char *)&buffer, length, false);
}

static int write_command(uint8_t reg, uint8_t value)
{
    uint8_t command[2];
    command[0] = reg;
    command[1] = value;

    return microbit_i2c_write(&microbit_i2c_obj, MMA8653_DEFAULT_ADDR, (const char *)command, 2, false);
}

static int16_t normalise(int8_t msb, int8_t lsb) {
    return ((msb<<2) + ((lsb>>6)&3))<<2;
}

static void update(microbit_accelerometer_obj_t *self) {
//    if (nrf_gpio_pin_read(MICROBIT_PIN_ACCEL_DATA_READY) == 0) {
        int8_t data[6];
        int err;

        err = read_command(MMA8653_OUT_X_MSB, (uint8_t *)data, 6);
        if (err < 6) {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C read failed with error code %d", err));
        }
        self->data->sample.x = normalise(data[0], data[1]);
        self->data->sample.y = normalise(data[2], data[3]);
        self->data->sample.z = normalise(data[4], data[5]);

        // TO DO -- Update gesture tracking

 //   }
}

mp_obj_t microbit_accelerometer_read_register(mp_obj_t self_in, mp_obj_t reg_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    uint8_t data;
    int reg = mp_obj_get_int(reg_in);
    int err = read_command(reg, &data, 1);
    if (err != 1) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_OSError, "I2C read failed with error code %d", err));
    }
    return mp_obj_new_int(data);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_accelerometer_read_register_obj, microbit_accelerometer_read_register);


void microbit_accelerometer_init(void) {

    microbit_accelerometer_obj.data->running = false;

    // First place the device into standby mode, so it can be configured.
    int err = write_command(MMA8653_CTRL_REG1, 0x00);
    if (err < 2)
        return;

    // Enable high precision mode. This consumes a bit more power, but still only 184 uA!
    err = write_command(MMA8653_CTRL_REG2, 0x10);
    if (err < 2)
        return;

    // Enable the INT1 interrupt pin.
    err = write_command(MMA8653_CTRL_REG4, 0x01);
    if (err < 2)
        return;

    // Select the DATA_READY event source to be routed to INT1
    err = write_command(MMA8653_CTRL_REG5, 0x01);
    if (err < 2)
        return;

    // Configure for 2g range.
    err = write_command(MMA8653_XYZ_DATA_CFG, 0);
    if (err < 2)
        return;

    // Bring the device back online, with 10bit wide samples at 50Hz.
    err = write_command(MMA8653_CTRL_REG1, 0x20 | 0x01);
    if (err < 2)
        return;

    microbit_accelerometer_obj.data->running = true;

}

mp_obj_t microbit_accelerometer_get_x(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->data->sample.x);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_x_obj, microbit_accelerometer_get_x);

mp_obj_t microbit_accelerometer_get_y(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->data->sample.y);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_y_obj, microbit_accelerometer_get_y);

mp_obj_t microbit_accelerometer_get_z(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->data->sample.z);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_z_obj, microbit_accelerometer_get_z);

mp_obj_t microbit_accelerometer_running(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->data->running);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_running_obj, microbit_accelerometer_running);

mp_obj_t microbit_accelerometer_get_values(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    mp_obj_tuple_t *tuple = (mp_obj_tuple_t *)mp_obj_new_tuple(3, NULL);
    update(self);
    tuple->items[0] = mp_obj_new_int(self->data->sample.x);
    tuple->items[1] = mp_obj_new_int(self->data->sample.y);
    tuple->items[2] = mp_obj_new_int(self->data->sample.z);
    return tuple;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_values_obj, microbit_accelerometer_get_values);

/*
STATIC const qstr gesture_name_map[] = {
    [GESTURE_NONE] = MP_QSTR_NULL,
    [GESTURE_UP] = MP_QSTR_up,
    [GESTURE_DOWN] = MP_QSTR_down,
    [GESTURE_LEFT] = MP_QSTR_left,
    [GESTURE_RIGHT] = MP_QSTR_right,
    [GESTURE_FACE_UP] = MP_QSTR_face_space_up,
    [GESTURE_FACE_DOWN] = MP_QSTR_face_space_down,
    [GESTURE_FREEFALL] = MP_QSTR_freefall,
    [GESTURE_3G] = MP_QSTR_3g,
    [GESTURE_6G] = MP_QSTR_6g,
    [GESTURE_8G] = MP_QSTR_8g,
    [GESTURE_SHAKE] = MP_QSTR_shake,
};
*/

/*
STATIC BasicGesture gesture_from_obj(mp_obj_t gesture_in) {
    qstr gesture = mp_obj_str_get_qstr(gesture_in);
    for (uint i = 0; i < MP_ARRAY_SIZE(gesture_name_map); ++i) {
        if (gesture == gesture_name_map[i]) {
            return (BasicGesture)i;
        }
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid gesture"));
}

mp_obj_t microbit_accelerometer_current_gesture(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    return MP_OBJ_NEW_QSTR(gesture_name_map[self->accelerometer->getGesture()]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_current_gesture_obj, microbit_accelerometer_current_gesture);

mp_obj_t microbit_accelerometer_is_gesture(mp_obj_t self_in, mp_obj_t gesture_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    BasicGesture gesture = gesture_from_obj(gesture_in);
    update(self);
    return mp_obj_new_bool(self->accelerometer->getGesture() == gesture);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_accelerometer_is_gesture_obj, microbit_accelerometer_is_gesture);

mp_obj_t microbit_accelerometer_was_gesture(mp_obj_t self_in, mp_obj_t gesture_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    BasicGesture gesture = gesture_from_obj(gesture_in);
    update(self);
    mp_obj_t result = mp_obj_new_bool(gesture_state & (1 << gesture));
    gesture_state &= (~(1 << gesture));
    gesture_list_cur = 0;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_accelerometer_was_gesture_obj, microbit_accelerometer_was_gesture);

mp_obj_t microbit_accelerometer_get_gestures(mp_obj_t self_in) {
    microbit_accelerometer_obj_t *self = (microbit_accelerometer_obj_t*)self_in;
    update(self);
    if (gesture_list_cur == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = (mp_obj_tuple_t*)mp_obj_new_tuple(gesture_list_cur, NULL);
    for (uint i = 0; i < gesture_list_cur; ++i) {
        uint gesture = (gesture_list[i >> 1] >> (4 * (i & 1))) & 0x0f;
        o->items[i] = MP_OBJ_NEW_QSTR(gesture_name_map[gesture]);
    }
    gesture_list_cur = 0;
    return o;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_accelerometer_get_gestures_obj, microbit_accelerometer_get_gestures);
*/

STATIC const mp_map_elem_t microbit_accelerometer_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_x), (mp_obj_t)&microbit_accelerometer_get_x_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_y), (mp_obj_t)&microbit_accelerometer_get_y_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_z), (mp_obj_t)&microbit_accelerometer_get_z_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_running), (mp_obj_t)&microbit_accelerometer_running_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_values), (mp_obj_t)&microbit_accelerometer_get_values_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_register), (mp_obj_t)&microbit_accelerometer_read_register_obj },
    // { MP_OBJ_NEW_QSTR(MP_QSTR_current_gesture), (mp_obj_t)&microbit_accelerometer_current_gesture_obj },
    // { MP_OBJ_NEW_QSTR(MP_QSTR_is_gesture), (mp_obj_t)&microbit_accelerometer_is_gesture_obj },
    // { MP_OBJ_NEW_QSTR(MP_QSTR_was_gesture), (mp_obj_t)&microbit_accelerometer_was_gesture_obj },
    // { MP_OBJ_NEW_QSTR(MP_QSTR_get_gestures), (mp_obj_t)&microbit_accelerometer_get_gestures_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_accelerometer_locals_dict, microbit_accelerometer_locals_dict_table);

const mp_obj_type_t microbit_accelerometer_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitAccelerometer,
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
    .locals_dict = (mp_obj_dict_t*)&microbit_accelerometer_locals_dict,
};

static microbit_accelerometer_data_t _data;

const microbit_accelerometer_obj_t microbit_accelerometer_obj = {
    {&microbit_accelerometer_type},
    .data = &_data
};

}
