#ifndef __MICROPY_INCLUDED_MICROBIT_INSTRUMENT_H__
#define __MICROPY_INCLUDED_MICROBIT_INSTRUMENT_H__

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
#include "py/runtime.h"

/* Waveform magnitude and scaling.
 * Internally all waveforms are conceptually a signed fixed point value in the range -1 to +1
 * This is represented by a machine (32 bit) integer scaled by 2**15. Note that we can accommodate values
 * outside the range -1..+1, but we need to be careful of overflow when multiplying.
 * All modulation (multiplication) must be scaled appropriately. Scaling should be done in such a
 * way as to minimise rounding losses. Typically x*y will implmented as (x*y)>>15, but we might, on occassion want to prescale.
 *
 * NOTE on shifts: In C/C++ signed right shift is undefined, so using shift to scale numbers depends on the
 * compiler. However, all major compilers do the sensible thing, so we rely on that.
 * Right shifting is equivelent to division, rounding towards -inf. So repeated shifting will drag the final
 * value slightly negative.
 *
 * Phase.
 * The starting value for each pipleine is the phase. Phases are scale by (2**31)/PI, so that one cycle is 2**32.
 * This will natuallly wrap, which keeps phase calculations very simple.
 */

#define SCALE_SHIFT 14
#define SCALED_ONE (1<< SCALE_SHIFT)
#define DOUBLE_SHIFT (SCALE_SHIFT<<1)

#ifdef __cplusplus
extern "C"
{
#endif

struct FilterComponent;

typedef int32_t (*filter_funcptr)(int32_t, struct FilterComponent *);
typedef void (*initialise_funcptr)(struct FilterComponent *);

struct EnvelopeSettings {
    int32_t attack_rate;
    int32_t decay_rate;
    int32_t sustain_level;
    int32_t release_rate;
};

struct LowPassFilterSettings {
    int32_t frequency;
    int32_t a;
    int32_t b;
};

struct Balance {
    int32_t left_volume;
    int32_t right_volume;

};

extern int32_t pseudo_sine_wave(int32_t, struct FilterComponent *);
extern int32_t square_wave(int32_t, struct FilterComponent *);
extern int32_t sawtooth_wave_rising(int32_t, struct FilterComponent *);
extern int32_t sawtooth_wave_falling(int32_t, struct FilterComponent *);
extern int32_t triangle_wave(int32_t, struct FilterComponent *);
extern int32_t step_wave(int32_t, struct FilterComponent *);
extern int32_t power4_wave(int32_t, struct FilterComponent *);
extern int32_t rectangular_wave(int32_t, struct FilterComponent *);
extern int32_t noise(int32_t, struct FilterComponent *);
extern int32_t zero(int32_t, struct FilterComponent *);
extern int32_t dc(int32_t, struct FilterComponent *);
extern int32_t fade_in(int32_t, struct FilterComponent *);


extern void ar_init(struct FilterComponent *f);
extern void adsr_init(struct FilterComponent *f);
extern void noop_init(struct FilterComponent *f);

extern int32_t float_to_fixed(float f, uint32_t scale);
extern float fixed_to_float(int32_t f);

#define SAMPLING_FREQUENCY 16384
//Scaling factor is (2**32)/SAMPLING_FREQUENCY/1000 == (2**30)/SAMPLING_FREQUENCY/250
// Which is 262 for 16kHz
#define MILLI_PHASE_SCALING ((1<<30)/SAMPLING_FREQUENCY/250)


extern const mp_obj_tuple_t component_base_tuple;

struct _monochrome_5by5_t;

typedef struct _microbit_oscillator_obj_t {
    mp_obj_base_t base;
    filter_funcptr waveform;
    mp_obj_t image;
    const char *name;
    int32_t edge;
} microbit_oscillator_obj_t;

extern const mp_obj_type_t microbit_oscillator_type;
extern const mp_obj_type_t microbit_lowpass_type;
extern const microbit_oscillator_obj_t microbit_sine_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_saw_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_square_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_triangle_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_step_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_power4_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_noise_oscillator_obj;
extern const microbit_oscillator_obj_t microbit_fade_in_obj;

typedef struct _microbit_instrument_obj_t {
    mp_obj_base_t base;
    int32_t detune1;
    mp_obj_t source1;
    mp_obj_t filter1;
    int32_t detune2;
    mp_obj_t source2;
    mp_obj_t filter2;
    struct Balance balance;
    mp_obj_t envelope;
} microbit_instrument_obj_t;

extern const microbit_instrument_obj_t microbit_bell_obj;
extern const microbit_instrument_obj_t microbit_drum_obj;
extern const microbit_instrument_obj_t microbit_buzzer_obj;
extern const microbit_instrument_obj_t microbit_organ_obj;
extern const microbit_instrument_obj_t microbit_strings_obj;

typedef struct _microbit_envelope_obj_t {
    mp_obj_base_t base;
    struct EnvelopeSettings settings;
    bool attack_release;
} microbit_envelope_obj_t;

extern const mp_obj_type_t microbit_component_type;
extern const mp_obj_type_t microbit_envelope_type;
extern const mp_obj_type_t microbit_settings_type;
extern const mp_obj_type_t microbit_instrument_type;

/* LowPass objects */

typedef struct _microbit_lowpass_obj_t {
    mp_obj_base_t base;
    struct LowPassFilterSettings  settings;
} microbit_lowpass_obj_t;

extern void initialise_component(mp_obj_t obj, struct FilterComponent *component);

extern int32_t lowpass(int32_t, struct FilterComponent *filter);

extern int32_t adsr_attack(int32_t, struct FilterComponent *envelope);
extern int32_t adsr_decay(int32_t, struct FilterComponent *envelope);
extern int32_t adsr_sustain(int32_t, struct FilterComponent *envelope);
extern int32_t adsr_release(int32_t, struct FilterComponent *envelope);
extern int32_t adsr_quiet(int32_t, struct FilterComponent *envelope);
extern int32_t ar_attack(int32_t, struct FilterComponent *envelope);

#ifdef __cplusplus
}
#endif

#endif // __MICROPY_INCLUDED_MICROBIT_INSTRUMENT_H__
