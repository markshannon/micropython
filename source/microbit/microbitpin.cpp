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

#include "MicroBit.h"
#include "microbitobj.h"

extern "C" {

#include "py/runtime.h"
#include "modmicrobit.h"
#include "lib/pwm.h"
#include "microbit/microbitpin.h"
#include "nrf_gpio.h"
#include "py/mphal.h"


static uint8_t microbit_pinmodes[] = {
    PINMODE_INDEX_UNUSED,  /* pin 0 */
    PINMODE_INDEX_UNUSED,  /* pin 1 */
    PINMODE_INDEX_UNUSED,  /* pin 2 */
    PINMODE_INDEX_DISPLAY, /* pin 3 */
    PINMODE_INDEX_DISPLAY, /* pin 4 */
    PINMODE_INDEX_BUTTON,  /* pin 5 - button A */
    PINMODE_INDEX_DISPLAY, /* pin 6 */
    PINMODE_INDEX_DISPLAY, /* pin 7 */
    PINMODE_INDEX_UNUSED,  /* pin 8 */
    PINMODE_INDEX_DISPLAY, /* pin 9 */
    PINMODE_INDEX_DISPLAY, /* pin 10 */
    PINMODE_INDEX_BUTTON,  /* pin 11 - button B */
    PINMODE_INDEX_UNUSED,  /* pin 12 */
    PINMODE_INDEX_SPI,     /* pin 13 */
    PINMODE_INDEX_SPI,     /* pin 14 */
    PINMODE_INDEX_SPI,     /* pin 15 */
    PINMODE_INDEX_UNUSED,  /* pin 16 */
    PINMODE_INDEX_3V,      /* pin 17 - 3V */
    PINMODE_INDEX_3V,      /* pin 18 - 3V */
    PINMODE_INDEX_I2C,     /* pin 19 */
    PINMODE_INDEX_I2C,     /* pin 20 */
};

static inline uint8_t microbit_obj_pin_get_mode(const microbit_pin_obj_t *pin) {
    return microbit_pinmodes[pin->number];
}

mp_obj_t microbit_pin_write_digital(mp_obj_t self_in, mp_obj_t value_in) {
    microbit_pin_obj_t *self = (microbit_pin_obj_t*)self_in;
    int val = mp_obj_get_int(value_in);
    if (val >> 1) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "value must be 0 or 1"));
    }
    if (microbit_obj_pin_get_mode(self) != PINMODE_INDEX_DIGITAL) {
        microbit_obj_pin_acquire(self, PINMODE_INDEX_DIGITAL);
        nrf_gpio_cfg_output(self->name);
    }
    if (val)
        nrf_gpio_pin_set(self->name);
    else
        nrf_gpio_pin_set(self->name);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_pin_write_digital_obj, microbit_pin_write_digital);

mp_obj_t microbit_pin_read_digital(mp_obj_t self_in) {
    microbit_pin_obj_t *self = (microbit_pin_obj_t*)self_in;
    if (!microbit_obj_pin_mode_allows_digital_read(self)) {
        microbit_obj_pin_acquire(self, PINMODE_INDEX_DIGITAL);
        nrf_gpio_cfg_input(self->name, NRF_GPIO_PIN_PULLDOWN);
    }
    return mp_obj_new_int(nrf_gpio_pin_read(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_pin_read_digital_obj, microbit_pin_read_digital);

mp_obj_t microbit_pin_write_analog(mp_obj_t self_in, mp_obj_t value_in) {
    microbit_pin_obj_t *self = (microbit_pin_obj_t*)self_in;
    int set_value;
    if (mp_obj_is_float(value_in)) {
        mp_float_t val = mp_obj_get_float(value_in);
        set_value = val+0.5;
    } else {
        set_value = mp_obj_get_int(value_in);
    }
    if (set_value < 0 || set_value > MICROBIT_PIN_MAX_OUTPUT) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "value must be between 0 and 1023"));
    }
    if (microbit_obj_pin_get_mode(self) != PINMODE_INDEX_ANALOG) {
        microbit_obj_pin_acquire(self, PINMODE_INDEX_ANALOG);
        nrf_gpio_cfg_output(self->name);
    }
    pwm_set_duty_cycle(self->name, set_value);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_pin_write_analog_obj, microbit_pin_write_analog);

mp_obj_t microbit_pin_read_analog(mp_obj_t self_in) {
    microbit_pin_obj_t *self = (microbit_pin_obj_t*)self_in;
    microbit_obj_pin_acquire(self, PINMODE_INDEX_UNUSED);
    analogin_t obj;
    analogin_init(&obj, self->name);
    int val = analogin_read_u16(&obj);
    return mp_obj_new_int(val);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_pin_read_analog_obj, microbit_pin_read_analog);

mp_obj_t microbit_pin_set_analog_period(mp_obj_t self_in, mp_obj_t period_in) {
    (void)self_in;
    int err = pwm_set_period_us(mp_obj_get_int(period_in)*1000);
    if (err) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid period"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_pin_set_analog_period_obj, microbit_pin_set_analog_period);

mp_obj_t microbit_pin_set_analog_period_microseconds(mp_obj_t self_in, mp_obj_t period_in) {
    (void)self_in;
    int err = pwm_set_period_us(mp_obj_get_int(period_in));
    if (err) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid period"));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_pin_set_analog_period_microseconds_obj, microbit_pin_set_analog_period_microseconds);

mp_obj_t microbit_pin_get_analog_period_microseconds(mp_obj_t self_in) {
    (void)self_in;
    int32_t period = pwm_get_period_us();
    return mp_obj_new_int(period);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_pin_get_analog_period_microseconds_obj, microbit_pin_get_analog_period_microseconds);

mp_obj_t microbit_pin_is_touched(mp_obj_t self_in) {
    microbit_pin_obj_t *self = (microbit_pin_obj_t*)self_in;
    uint8_t mode = microbit_obj_pin_get_mode(self);
    if (mode != PINMODE_INDEX_TOUCH && mode != PINMODE_INDEX_BUTTON) {
        microbit_obj_pin_acquire(self, PINMODE_INDEX_TOUCH);
        nrf_gpio_cfg_input(self->name, NRF_GPIO_PIN_PULLDOWN);
        // Give debounce a few cycles to settle
        mp_hal_delay_ms(20);
    }
    return mp_obj_new_bool(microbit_pin_debounced(self->name));
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_pin_is_touched_obj, microbit_pin_is_touched);

STATIC const mp_map_elem_t microbit_dig_pin_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_digital), (mp_obj_t)&microbit_pin_write_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_digital), (mp_obj_t)&microbit_pin_read_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_analog), (mp_obj_t)&microbit_pin_write_analog_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period), (mp_obj_t)&microbit_pin_set_analog_period_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period_microseconds), (mp_obj_t)&microbit_pin_set_analog_period_microseconds_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_analog_period_microseconds), (mp_obj_t)&microbit_pin_get_analog_period_microseconds_obj },
};

STATIC const mp_map_elem_t microbit_ann_pin_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_digital), (mp_obj_t)&microbit_pin_write_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_digital), (mp_obj_t)&microbit_pin_read_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_analog), (mp_obj_t)&microbit_pin_write_analog_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_analog), (mp_obj_t)&microbit_pin_read_analog_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period), (mp_obj_t)&microbit_pin_set_analog_period_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period_microseconds), (mp_obj_t)&microbit_pin_set_analog_period_microseconds_obj },
};

STATIC const mp_map_elem_t microbit_touch_pin_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_digital), (mp_obj_t)&microbit_pin_write_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_digital), (mp_obj_t)&microbit_pin_read_digital_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write_analog), (mp_obj_t)&microbit_pin_write_analog_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_analog), (mp_obj_t)&microbit_pin_read_analog_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period), (mp_obj_t)&microbit_pin_set_analog_period_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_analog_period_microseconds), (mp_obj_t)&microbit_pin_set_analog_period_microseconds_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_touched), (mp_obj_t)&microbit_pin_is_touched_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_dig_pin_locals_dict, microbit_dig_pin_locals_dict_table);
STATIC MP_DEFINE_CONST_DICT(microbit_ann_pin_locals_dict, microbit_ann_pin_locals_dict_table);
STATIC MP_DEFINE_CONST_DICT(microbit_touch_pin_locals_dict, microbit_touch_pin_locals_dict_table);


const mp_obj_type_t microbit_dig_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitDigitalPin,
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
    .locals_dict = (mp_obj_dict_t*)&microbit_dig_pin_locals_dict,
};

const mp_obj_type_t microbit_ad_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitAnalogDigitalPin,
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
    .locals_dict = (mp_obj_dict_t*)&microbit_ann_pin_locals_dict,
};

const mp_obj_type_t microbit_touch_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitTouchPin,
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
    .locals_dict = (mp_obj_dict_t*)&microbit_touch_pin_locals_dict,
};

const microbit_pin_obj_t microbit_p0_obj = {{&microbit_touch_pin_type}, 0, uBit.io.P0.name};
const microbit_pin_obj_t microbit_p1_obj = {{&microbit_touch_pin_type}, 1, uBit.io.P1.name};
const microbit_pin_obj_t microbit_p2_obj = {{&microbit_touch_pin_type}, 2, uBit.io.P2.name};
const microbit_pin_obj_t microbit_p3_obj = {{&microbit_ad_pin_type}, 3, uBit.io.P3.name};
const microbit_pin_obj_t microbit_p4_obj = {{&microbit_ad_pin_type}, 4, uBit.io.P4.name};
const microbit_pin_obj_t microbit_p5_obj = {{&microbit_dig_pin_type}, 5, uBit.io.P5.name};
const microbit_pin_obj_t microbit_p6_obj = {{&microbit_dig_pin_type}, 6, uBit.io.P6.name};
const microbit_pin_obj_t microbit_p7_obj = {{&microbit_dig_pin_type}, 7, uBit.io.P7.name};
const microbit_pin_obj_t microbit_p8_obj = {{&microbit_dig_pin_type}, 8, uBit.io.P8.name};
const microbit_pin_obj_t microbit_p9_obj = {{&microbit_dig_pin_type}, 9, uBit.io.P9.name};
const microbit_pin_obj_t microbit_p10_obj = {{&microbit_ad_pin_type}, 10, uBit.io.P10.name};
const microbit_pin_obj_t microbit_p11_obj = {{&microbit_dig_pin_type}, 11, uBit.io.P11.name};
const microbit_pin_obj_t microbit_p12_obj = {{&microbit_dig_pin_type}, 12, uBit.io.P12.name};
const microbit_pin_obj_t microbit_p13_obj = {{&microbit_dig_pin_type}, 13, uBit.io.P13.name};
const microbit_pin_obj_t microbit_p14_obj = {{&microbit_dig_pin_type}, 14, uBit.io.P14.name};
const microbit_pin_obj_t microbit_p15_obj = {{&microbit_dig_pin_type}, 15, uBit.io.P15.name};
const microbit_pin_obj_t microbit_p16_obj = {{&microbit_dig_pin_type}, 16, uBit.io.P16.name};
const microbit_pin_obj_t microbit_p19_obj = {{&microbit_dig_pin_type}, 19, uBit.io.P19.name};
const microbit_pin_obj_t microbit_p20_obj = {{&microbit_dig_pin_type}, 20, uBit.io.P20.name};

static const microbit_pinmode_t pinmodes[] = {
    {
        MP_QSTR_unused, true, true, NULL,
    },
    {
        MP_QSTR_analog, true, false, pwm_release_pin,
    },
    {
        MP_QSTR_digital, true, true, NULL,
    },
    {
        MP_QSTR_display, false, true, NULL,
    },
    {
        MP_QSTR_button, false, true, NULL,
    },
    {
        MP_QSTR_touch, true, true, NULL,
    },
    {
        MP_QSTR_audio, false, false, NULL,
    },
    {
        MP_QSTR_i2c, false, false, NULL,
    },
    {
        MP_QSTR_spi, false, false, NULL,
    },
    {
        MP_QSTR_3v, false, false, NULL,
    },
};

void microbit_obj_pin_set_mode(const microbit_pin_obj_t *pin, uint8_t new_mode) {
    microbit_pinmodes[pin->number] = new_mode;
}

bool microbit_obj_pin_mode_allows_digital_read(const microbit_pin_obj_t *pin) {
    return pinmodes[microbit_obj_pin_get_mode(pin)].allows_digital_read;
}

void microbit_obj_pin_fail_if_cant_acquire(const microbit_pin_obj_t *pin) {
    const microbit_pinmode_t *current_mode = &pinmodes[microbit_obj_pin_get_mode(pin)];
    if (!current_mode->acquireable) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError, "Pin in %q mode", current_mode->name));
    }
    if (current_mode->release != NULL) {
        current_mode->release(pin->name);
    }
}

void microbit_obj_pin_acquire(const microbit_pin_obj_t *pin, uint8_t new_mode) {
    microbit_obj_pin_fail_if_cant_acquire(pin);
    microbit_obj_pin_set_mode(pin, new_mode);
}

microbit_pin_obj_t *microbit_obj_get_pin(mp_obj_t o) {
    mp_obj_type_t *type = mp_obj_get_type(o);
    if (type == &microbit_touch_pin_type || type == &microbit_ad_pin_type || type == &microbit_dig_pin_type) {
        return (microbit_pin_obj_t*)o;
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "expecting a pin"));
    }
}

PinName microbit_obj_get_pin_name(mp_obj_t o) {
    return microbit_obj_get_pin(o)->name;
}

}