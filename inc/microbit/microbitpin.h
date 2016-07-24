/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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
#ifndef __MICROPY_INCLUDED_MICROBIT_MICROBITPIN_H__
#define __MICROPY_INCLUDED_MICROBIT_MICROBITPIN_H__

#ifndef MICROBIT_PIN_P0

#define MICROBIT_PIN_P0     (P0_3)
#define MICROBIT_PIN_P1     (P0_2)
#define MICROBIT_PIN_P13    (P0_23)
#define MICROBIT_PIN_P14    (P0_22)
#define MICROBIT_PIN_P15    (P0_21)

#endif

#include "microbit/microbitobj.h"

mp_obj_t microbit_pin_write_digital(mp_obj_t self_in, mp_obj_t value_in);
mp_obj_t microbit_pin_read_digital(mp_obj_t self_in);

typedef void (*release_funcptr)(PinName pin);

typedef struct _pinmode {
    qstr name;
    bool acquireable;
    bool allows_digital_read;
    release_funcptr release;
} microbit_pinmode_t;

#define PINMODE_INDEX_UNUSED 0
#define PINMODE_INDEX_ANALOG 1
#define PINMODE_INDEX_DIGITAL 2
#define PINMODE_INDEX_DISPLAY 3
#define PINMODE_INDEX_BUTTON 4
#define PINMODE_INDEX_TOUCH 5
#define PINMODE_INDEX_AUDIO 6
#define PINMODE_INDEX_I2C 7
#define PINMODE_INDEX_SPI 8
#define PINMODE_INDEX_3V 9

void microbit_obj_pin_set_mode(const microbit_pin_obj_t *pin, uint8_t new_mode);

void microbit_obj_pin_fail_if_cant_acquire(const microbit_pin_obj_t *pin);

void microbit_obj_pin_acquire(const microbit_pin_obj_t *pin, uint8_t new_mode);

bool microbit_obj_pin_mode_allows_digital_read(const microbit_pin_obj_t *pin);

bool microbit_pin_debounced(PinName name);

#endif // __MICROPY_INCLUDED_MICROBIT_MICROBITPIN_H__
