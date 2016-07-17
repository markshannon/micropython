#ifndef __MICROPY_INCLUDED_MICROBIT_SYNTHESISER_H__
#define __MICROPY_INCLUDED_MICROBIT_SYNTHESISER_H__

/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Mark Shannon
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

#include <stdint.h>
#include <stdio.h>
#include "modaudio.h"

extern "C" {

/** This should be merged into AudioFrame, to allow 16bit signed PCM as well. */

typedef struct _microbit_synth_frame_t {
    mp_obj_base_t base;
    int16_t data[AUDIO_CHUNK_SIZE];
} microbit_synth_frame_t;


typedef int16_t (*wavefunc_t)(int16_t phase, microbit_synth_frame_t *frame);

typedef enum {
    MERGE_OP_ADD,
    MERGE_OP_SUBTRACT,
    MERGE_OP_MULTIPLY
} merge_op_t;

typedef struct _prng_state_t {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
} prng_state_t;

typedef struct _microbit_synth_level_t {
    mp_obj_base_t base;
    int16_t value;
    microbit_synth_frame_t *output;
} microbit_synth_level_t;

typedef struct _microbit_synth_merge_t {
    mp_obj_base_t base;
    mp_obj_t left;
    mp_obj_t right;
} microbit_synth_merge_t;

extern int16_t noise(int16_t ignore, microbit_synth_frame_t *frame);
extern int16_t sine_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t sawtooth_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t triangle_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t square_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t rectangle1_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t rectangle2_wave(int16_t phase, microbit_synth_frame_t *frame);
extern int16_t rectangle3_wave(int16_t phase, microbit_synth_frame_t *frame);

typedef struct _microbit_synth_waveform_t {
    mp_obj_base_t base;
    mp_obj_t input;
    int16_t phase;
    wavefunc_t waveform;
} microbit_synth_waveform_t;

typedef int16_t (*adsr_state_func_t)(int16_t signal, struct _microbit_synth_envelope_t *env);

typedef struct _microbit_synth_envelope_t {
    mp_obj_base_t base;
    int16_t attack_rate;
    int16_t decay_rate;
    int16_t sustain_level;
    int16_t release_rate;
    int16_t level;
    adsr_state_func_t state;
    mp_obj_t input;
} microbit_synth_envelope_t;

typedef struct _microbit_synth_lowpass_t {
    mp_obj_base_t base;
    int32_t a;
    int32_t b;
    int32_t z1;
    mp_obj_t input;
} microbit_synth_lowpass_t;

typedef struct _microbit_synth_chorus_t {
    mp_obj_base_t base;
    mp_obj_t input;
    int16_t last_output;
    int16_t delay;
    int16_t bucket_mask;
    int16_t index;
    int16_t volume;
    int16_t feedback;
    int16_t *buckets;
} microbit_synth_chorus_t;

extern uint8_t hard_cutoff(int16_t);
extern uint8_t soft_cutoff(int16_t);

typedef struct _microbit_synth_output_t {
    mp_obj_base_t base;
    mp_obj_t input;
    microbit_audio_frame_obj_t *output;
} microbit_synth_output_t;

}

#endif // __MICROPY_INCLUDED_MICROBIT_SYNTHESISER_H__