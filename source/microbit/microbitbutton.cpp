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

extern "C" {

#include "py/runtime.h"
#include "microbitobj.h"
#include "modmicrobit.h"
#include "nrf_gpio.h"

typedef struct _microbit_button_obj_t {
    mp_obj_base_t base;
    /* Stores pressed count in top 31 bits and was_pressed in the low bit */
    PinName name;
    uint8_t index;
} microbit_button_obj_t;

static mp_uint_t pressed[2];
static int8_t sigmas[8];
static int8_t debounced[8];

mp_obj_t microbit_button_is_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    return mp_obj_new_bool(debounced[self->name]);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_is_pressed_obj, microbit_button_is_pressed);


mp_obj_t microbit_button_get_presses(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    mp_obj_t n_presses = mp_obj_new_int(pressed[self->index] >> 1);
    pressed[self->index] &= 1;
    return n_presses;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_get_presses_obj, microbit_button_get_presses);

mp_obj_t microbit_button_was_pressed(mp_obj_t self_in) {
    microbit_button_obj_t *self = (microbit_button_obj_t*)self_in;
    mp_int_t presses = pressed[self->index];
    mp_obj_t result = mp_obj_new_bool(presses & 1);
    pressed[self->index] = presses & -2;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_button_was_pressed_obj, microbit_button_was_pressed);

void microbit_button_init(void) {
}

STATIC const mp_map_elem_t microbit_button_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_pressed), (mp_obj_t)&microbit_button_is_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_was_pressed), (mp_obj_t)&microbit_button_was_pressed_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_presses), (mp_obj_t)&microbit_button_get_presses_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_button_locals_dict, microbit_button_locals_dict_table);

STATIC const mp_obj_type_t microbit_button_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitButton,
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
    .locals_dict = (mp_obj_dict_t*)&microbit_button_locals_dict,
};

const microbit_button_obj_t microbit_button_a_obj = {
    {&microbit_button_type},
    .name = microbit_p5_obj.name,
    .index = 0,
};

const microbit_button_obj_t microbit_button_b_obj = {
    {&microbit_button_type},
    .name = microbit_p11_obj.name,
    .index = 1,
};

static bool update(PinName name) {
    int not_pressed = nrf_gpio_pin_read(name);
    int8_t sigma = sigmas[name&7] + 1-((not_pressed)<<1);
    sigmas[name&7] = sigma;
    if (sigma < 3) {
        if (sigma < 0)
            sigma = 0;
        debounced[name&7] = false;
        return not_pressed == 0;
    } else if (sigma > 7) {
        if (sigma > 12)
            sigma = 12;
        debounced[name&7] = true;
        return not_pressed != 0;
    }
    return false;
}

void microbit_button_tick(void) {
    // Update both buttons and the touch pins.
    if (update(microbit_button_a_obj.name))
        pressed[microbit_button_a_obj.index] = (pressed[microbit_button_a_obj.index] + 2) | 1;
    if (update(microbit_button_b_obj.name))
        pressed[microbit_button_b_obj.index] = (pressed[microbit_button_b_obj.index] + 2) | 1;
    update(microbit_p0_obj.name);
    update(microbit_p1_obj.name);
    update(microbit_p2_obj.name);
}

bool microbit_pin_debounced(PinName name) {
    return debounced[name&7];
}

}