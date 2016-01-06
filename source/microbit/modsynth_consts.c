/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Mark Shannon
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

#include "py/runtime.h"
#include "microbit/instrument.h"
#include "microbit/modmicrobit.h"

/* Component class -- base class for Oscillator and Filter. */

const mp_obj_type_t microbit_component_type = {
    { &mp_type_type },
    .name = MP_QSTR_Component,
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
    .bases_tuple = MP_OBJ_NULL,
    /* .locals_dict = */ MP_OBJ_NULL,
};

const microbit_oscillator_obj_t microbit_sine_oscillator_obj = {
    { &microbit_oscillator_type },
    &pseudo_sine_wave,
    (mp_obj_t)&microbit_const_image_sine_wave_obj,
    "sine",
    0
};

const microbit_oscillator_obj_t microbit_saw_oscillator_obj = {
    { &microbit_oscillator_type },
    &sawtooth_wave_falling,
    (mp_obj_t)&microbit_const_image_saw_wave_obj,
    "saw",
    0
};

const microbit_oscillator_obj_t microbit_square_oscillator_obj = {
    { &microbit_oscillator_type },
    &square_wave,
    (mp_obj_t)&microbit_const_image_square_wave_obj,
    "square",
    0
};

const microbit_oscillator_obj_t microbit_triangle_oscillator_obj = {
    { &microbit_oscillator_type },
    &triangle_wave,
    (mp_obj_t)&microbit_const_image_triangle_wave_obj,
    "triangle",
    0
};

const microbit_oscillator_obj_t microbit_step_oscillator_obj = {
    { &microbit_oscillator_type },
    &step_wave,
    (mp_obj_t)&microbit_const_image_step_wave_obj,
    "step",
    0
};

const microbit_oscillator_obj_t microbit_power4_oscillator_obj = {
    { &microbit_oscillator_type },
    &power4_wave,
    (mp_obj_t)&microbit_const_image_power4_wave_obj,
    "power4",
    0
};

const microbit_oscillator_obj_t microbit_fade_in_obj = {
    { &microbit_oscillator_type },
    &noise,
    (mp_obj_t)&microbit_const_image_fade_in_obj,
    "fade_in",
    0
};

const microbit_oscillator_obj_t microbit_noise_oscillator_obj = {
    { &microbit_oscillator_type },
    &noise,
    (mp_obj_t)&microbit_const_image_noise_wave_obj,
    "noise",
    0
};

const mp_obj_tuple_t component_base_tuple = {{&mp_type_tuple}, 1, {(mp_obj_t)&microbit_component_type}};

#define SCALE_FLOAT(f) ((int32_t)((1<<SCALE_SHIFT)*f))
#define SCALE_RATE(dur, frac)  ((int32_t)((((1<<DOUBLE_SHIFT)*frac))/(dur*16)))
#define SUSTAIN(s) ((1<<DOUBLE_SHIFT)*s)

const microbit_envelope_obj_t microbit_bell_adsr_obj = {
    .base = { &microbit_envelope_type },
    .settings = { SCALE_RATE(1, 1), SCALE_RATE(1500, 1), 1, SCALE_RATE(1500, 1) },
    .attack_release = false,
};

const microbit_instrument_obj_t microbit_bell_obj = {
    .base = { &microbit_instrument_type },
    .detune1 = SCALE_FLOAT(1),
    .source1 = (mp_obj_t)&microbit_sine_oscillator_obj,
    .detune2 = SCALE_FLOAT(5),
    .source2 = (mp_obj_t)&microbit_sine_oscillator_obj,
    .filter1 = MP_OBJ_NULL,
    .filter2 = MP_OBJ_NULL,
    .balance = { SCALE_FLOAT(0.7), SCALE_FLOAT(0.3) },
    .envelope = (mp_obj_t)&microbit_bell_adsr_obj,
};

const microbit_envelope_obj_t microbit_organ_adsr_obj = {
    .base = { &microbit_envelope_type },
    .settings = { SCALE_RATE(50, 1), SCALE_RATE(40, 0.2), SUSTAIN(0.8), SCALE_RATE(400, 0.8) },
    .attack_release = false,
};

const microbit_instrument_obj_t microbit_organ_obj = {
    .base = { &microbit_instrument_type },
    .detune1 = SCALE_FLOAT(1),
    .source1 = (mp_obj_t)&microbit_sine_oscillator_obj,
    .detune2 = SCALE_FLOAT(2),
    .source2 = (mp_obj_t)&microbit_sine_oscillator_obj,
    .filter1 = MP_OBJ_NULL,
    .filter2 = MP_OBJ_NULL,
    .balance = { SCALE_FLOAT(0.8), SCALE_FLOAT(0.2) },
    .envelope = (mp_obj_t)&microbit_organ_adsr_obj,
};

const microbit_envelope_obj_t microbit_drum_adsr_obj = {
    .base = { &microbit_envelope_type },
    .settings = { SCALE_RATE(1, 1), SCALE_RATE(500, 1), 1, SCALE_RATE(500, 1) },
    .attack_release = false,
};

const microbit_envelope_obj_t microbit_drum_noise_ar_obj = {
    .base = { &microbit_envelope_type },
    .settings = { 1<<30, 0, 0, SCALE_RATE(80, 1) },
    .attack_release = true,
};

const microbit_instrument_obj_t microbit_drum_obj = {
    .base = { &microbit_instrument_type },
    .detune1 = SCALE_FLOAT(1),
    .source1 = (mp_obj_t)&microbit_noise_oscillator_obj,
    .filter1 = (mp_obj_t)&microbit_drum_noise_ar_obj,
    .detune2 = SCALE_FLOAT(1),
    .source2 = (mp_obj_t)&microbit_sine_oscillator_obj,
    .filter2 = MP_OBJ_NULL,
    .balance = { SCALE_FLOAT(0.5), SCALE_FLOAT(0.9) },
    .envelope = (mp_obj_t)&microbit_drum_adsr_obj,
};

const microbit_envelope_obj_t microbit_square_adsr_obj = {
    .base = { &microbit_envelope_type },
    .settings = { SCALE_RATE(1, 1), 0, SUSTAIN(1), SCALE_RATE(1, 1) },
    .attack_release = false,
};

const microbit_instrument_obj_t microbit_buzzer_obj = {
    .base = { &microbit_instrument_type },
    .detune1 = SCALE_FLOAT(1),
    .source1 = (mp_obj_t)&microbit_square_oscillator_obj,
    .detune2 = SCALE_FLOAT(1),
    .source2 = (mp_obj_t)&microbit_square_oscillator_obj,
    .filter1 = MP_OBJ_NULL,
    .filter2 = MP_OBJ_NULL,
    .balance = { SCALE_FLOAT(1), SCALE_FLOAT(0) },
    .envelope = (mp_obj_t)&microbit_square_adsr_obj,
};

const microbit_envelope_obj_t microbit_string_adsr_obj = {
    .base = { &microbit_envelope_type },
    .settings = { SCALE_RATE(80, 1), SCALE_RATE(40, 0.1), SUSTAIN(0.9), SCALE_RATE(400, 0.9) },
    .attack_release = false,
};

#define EDGE_FROM_DUTY(d) ((-1<<31)+(((1<<30)/10)*(d/5))+8)

static const microbit_oscillator_obj_t pwm40 = {
    .base = { &microbit_oscillator_type },
    .waveform = &rectangular_wave,
    .image = mp_const_none,
    "",
    EDGE_FROM_DUTY(40),
};

static const microbit_oscillator_obj_t pwm60 = {
    .base = { &microbit_oscillator_type },
    .waveform = &rectangular_wave,
    .image = mp_const_none,
    "",
    EDGE_FROM_DUTY(60),
};

const microbit_instrument_obj_t microbit_strings_obj = {
    .base = { &microbit_instrument_type },
    .detune1 = SCALE_FLOAT(1.1),
    .source1 = (mp_obj_t)&pwm40,
    .detune2 = SCALE_FLOAT(0.9),
    .source2 = (mp_obj_t)&pwm60,
    .filter1 = MP_OBJ_NULL,
    .filter2 = MP_OBJ_NULL,
    .balance = { SCALE_FLOAT(0.5), SCALE_FLOAT(0.5) },
    .envelope = (mp_obj_t)&microbit_string_adsr_obj,
};
