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


#include "microbit/synthesiser.h"
#include <stdio.h>
#include <cstddef>

extern "C" {

static inline uint32_t milliHertzToPhaseDelta(uint32_t mHz) {
    return mHz * MILLI_PHASE_SCALING;
}

Synth *theSynth = NULL;

static inline int32_t get_phase(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = s->oscillator.phase + phase_delta;
    s->oscillator.phase = phase;
    return phase;
}

int32_t pseudo_sine_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    /* magnitude = 2x - x**2 */
    int nsign = 1-((phase>>30)&2);
    int32_t x = (((uint32_t)phase)<<1)>>(31-SCALE_SHIFT);
    int32_t x2 = (x*x)>>SCALE_SHIFT;
    int32_t magnitude = x + x - x2;
    return nsign*magnitude;
}

#define SCALED_TWO (SCALED_ONE<<1)

int32_t power4_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    int sign = ((phase>>30)&2)-1;
    int32_t x = (((uint32_t)phase)<<1)>>(31-SCALE_SHIFT);
    int32_t xm2 = x - SCALED_TWO;
    /* magnitude = ((x-2)*x+2)*(x-2)*x */
    int32_t magnitude = (((((((xm2*x)>>SCALE_SHIFT)+SCALED_TWO)*x)>>SCALE_SHIFT)*xm2)>>SCALE_SHIFT);
    return sign*magnitude;
}

int32_t square_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    int32_t sign = ((((uint32_t)phase)>>31)<<1)-1;
    return sign<<SCALE_SHIFT;
}

int32_t sawtooth_wave_rising(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    return (((uint32_t)phase)>>(31-SCALE_SHIFT))-SCALED_ONE;
}

int32_t sawtooth_wave_falling(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    return (((uint32_t)phase)>>(31-SCALE_SHIFT))-SCALED_ONE;
}

int32_t triangle_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    int32_t sign = ((((uint32_t)phase)>>31)<<1)-1;
    return ((sign*phase)>>(30-SCALE_SHIFT))+SCALED_ONE;
}

int32_t step_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    int32_t step = (phase>>29);
    int32_t sign = ((((uint32_t)phase)>>31)<<1)-1;
    return ((sign*step)<<(SCALE_SHIFT-1))+SCALED_ONE;
}

int32_t rectangular_wave(int32_t phase_delta, FilterComponent *s) {
    int32_t phase = get_phase(phase_delta, s);
    if (phase < s->oscillator.edge) {
        return SCALED_ONE;
    } else {
        return -SCALED_ONE;
    }
}

/* Special wave used for changing a level very smoothly.
 * Starting with phase 0 it does half a cycle from 0 to +1
 * and then locks up at +1
 */
int32_t fade_in(int32_t phase_delta, FilterComponent *s) {
    if (s->oscillator.phase < 0)
        return SCALED_ONE;
    int32_t phase = get_phase(phase_delta, s);
    int32_t x = (phase<<1)>>(31-SCALE_SHIFT);
    int32_t x2 = (x*x)>>(SCALE_SHIFT+1);
    if (phase > (1<<30)) {
        return SCALED_ONE-x2;
    } else {
        return x2;
    }
}

static uint32_t xorDelta128(FilterComponent *s) {
    uint32_t x = s->prng.x;
    uint32_t y = s->prng.y;
    uint32_t z = s->prng.z;
    uint32_t w = s->prng.w;

    uint32_t t = x ^ (x << 11);
    s->prng.x = y;
    s->prng.y = z;
    s->prng.z = w;
    return s->prng.w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

int32_t noise(int32_t phase_delta __attribute__ ((unused)), FilterComponent *s) {
    return ((int32_t)xorDelta128(s))>>(31-SCALE_SHIFT);
}

int32_t zero(int32_t phase_delta __attribute__ ((unused)), FilterComponent *s __attribute__ ((unused))) {
    return 0;
}

int32_t dc(int32_t phase_delta __attribute__ ((unused)), FilterComponent *s __attribute__ ((unused))) {
    return SCALED_ONE;
}

union _i2f {
    int32_t bits;
    float value;
};

int32_t lowpass(int32_t signal, FilterComponent *filter) {
    int32_t z1 = filter->lowpass.z1;
    int32_t z0 = signal + ((filter->lowpass.a*z1)>>SCALE_SHIFT);
    int32_t out = ((filter->lowpass.b*(z0+z1))>>SCALE_SHIFT);
    filter->lowpass.z1 = z0;
    return out;
}

void ar_init(struct FilterComponent *f) {
    f->envelope.level = 0;
    f->action = ar_attack;
    f->envelope.active = true;
}

void adsr_init(struct FilterComponent *f) {
    f->envelope.level = 0;
    f->action = adsr_attack;
    f->envelope.active = true;
}

void noop_init(struct FilterComponent *f __attribute__ ((unused))) {
}

int32_t adsr_quiet(int32_t signal __attribute__ ((unused)), FilterComponent *env) {
    env->envelope.active = false;
    return 0;
}

int32_t adsr_release(int32_t signal, FilterComponent *env) {
    int32_t new_level =  env->envelope.level - env->envelope.settings.release_rate;
    if (new_level < 0) {
        new_level = 0;
        env->action = adsr_quiet;
    }
    env->envelope.level = new_level;
    return (signal*(new_level>>SCALE_SHIFT))>>SCALE_SHIFT;
}

int32_t adsr_sustain(int32_t signal, FilterComponent *env) {
    return (signal*(env->envelope.level>>SCALE_SHIFT))>>SCALE_SHIFT;
}

int32_t adsr_decay(int32_t signal, FilterComponent *env) {
    int32_t new_level =  env->envelope.level- env->envelope.settings.decay_rate;
    if (new_level < env->envelope.settings.sustain_level) {
        new_level = env->envelope.settings.sustain_level;
        env->action = adsr_sustain;
    }
    env->envelope.level = new_level;
    return (signal*(new_level>>SCALE_SHIFT))>>SCALE_SHIFT;
}

int32_t adsr_attack(int32_t signal, FilterComponent *env) {
    int32_t new_level =  env->envelope.level + env->envelope.settings.attack_rate;
    if ((new_level<<(31-DOUBLE_SHIFT)) < 0) {
        new_level = 1 << DOUBLE_SHIFT;
        env->action = adsr_decay;
    }
    env->envelope.level = new_level;
    return (signal*(new_level>>SCALE_SHIFT))>>SCALE_SHIFT;
}

int32_t ar_attack(int32_t signal, FilterComponent *env) {
    int32_t new_level =  env->envelope.level + env->envelope.settings.attack_rate;
    if ((new_level<<(31-DOUBLE_SHIFT)) < 0) {
        new_level = 1 << DOUBLE_SHIFT;
        env->action = adsr_release;
    }
    env->envelope.level = new_level;
    return (signal*(new_level>>SCALE_SHIFT))>>SCALE_SHIFT;
}

int32_t lfo(int32_t signal __attribute__ ((unused)), int32_t control __attribute__ ((unused)), FilterComponent *lfo) {
    return triangle_wave(lfo->lfo.phase_delta, lfo);
}

int32_t chorus(int32_t signal, int32_t control, FilterComponent *chorus) {
    // Control is scaled ms. So we rescale to steps.
    int32_t int_delay = control >> (SCALE_SHIFT-4);
    int32_t part_delay = control - (int_delay<<(SCALE_SHIFT-4));
    int32_t index = chorus->chorus.index;
    int32_t mask = chorus->chorus.bucket_mask;
    int32_t val0 = chorus->chorus.buckets[(index+int_delay)&mask];
    int32_t val1 = chorus->chorus.buckets[(index+int_delay+1)&mask];
    int32_t val = val0*((1<<(SCALE_SHIFT-4))-part_delay) + val1*part_delay;
    // Account for 2 shift in storage.
    val >>= (SCALE_SHIFT-2);
    val = (val*chorus->chorus.volume)>>SCALE_SHIFT;
    // Shift by 2 to avoid overflow
    chorus->chorus.buckets[index] = signal + val >> 2;
    index = (index+1)&mask;
    return signal + val;
}

void Voice::release() {
    this->envelope.action = adsr_release;
}


int32_t layout_a(Voice * v) {
    int32_t left = v->source1.action(v->phase_delta1, &v->source1);
    left = v->filter1.action(left, &v->filter1);
    int32_t right = v->source2.action(v->phase_delta2, &v->source2);
    right = v->filter2.action(right, &v->filter2);
    int32_t out = (((int32_t)v->balance.left_volume)*left+((int32_t)v->balance.right_volume)*right)>>SCALE_SHIFT;
    return v->envelope.action(out, &v->envelope);
}


static uint32_t frequency_shift(uint32_t mHz, uint32_t mult) {
    /* mHz can have a *very* large range from wow-wow as low as 0.2 Hz
     * up to the max niquist limit of 8kHz. So we need to be careful not to
     * loose precision
     */
    // May have to throw away some info -- Keep top bits of mHz and all of mult.
    int remaining_shift = SCALE_SHIFT;
    while (mHz>=(1<<(29-SCALE_SHIFT))) {
        mHz >>= 1;
        --remaining_shift;
    }
    return (mHz*mult)>>remaining_shift;
}

void Voice::start(uint32_t mHz) {
    this->phase_delta1 = milliHertzToPhaseDelta(frequency_shift(mHz, this->detune1));
    this->phase_delta2 = milliHertzToPhaseDelta(frequency_shift(mHz, this->detune2));
    this->source1.init(&this->source1);
    this->source2.init(&this->source2);
    this->filter1.init(&this->filter1);
    this->filter2.init(&this->filter2);
    this->envelope.init(&this->envelope);
}

void Voice::setInstrument(microbit_instrument_obj_t *instrument) {
    this->detune1 = instrument->detune1;
    this->detune2 = instrument->detune2;
    this->balance = instrument->balance;
    initialise_component(instrument->source1, &this->source1);
    initialise_component(instrument->source2, &this->source2);
    initialise_component(instrument->filter1, &this->filter1);
    initialise_component(instrument->filter2, &this->filter2);
    initialise_component(instrument->envelope, &this->envelope);
}

Voice* Synth::start(microbit_instrument_obj_t *instrument, uint32_t mHz) {
    // Use eldest if no inactive voice.
    Voice *voice = this->voice_ordering[VOICE_COUNT-1];
    int i = 0;
    for(; i < VOICE_COUNT-1; i++) {
        if (!this->voice_ordering[i]->envelope.envelope.active) {
            voice = this->voice_ordering[i];
            break;
        }
    }
    for (; i > 0; --i) {
        this->voice_ordering[i] =  this->voice_ordering[i-1];
    }
    this->voice_ordering[0] = voice;
    voice->setInstrument(instrument);
    voice->start(mHz);
    return voice;
}

Synth::Synth() {
    for (int i = 0; i < VOICE_COUNT; i++) {
        this->voice_ordering[i] = &this->voices[i];
    }
    this->reset();
    this->out_level = SCALED_ONE/2;
}

void Synth::reset() {
    for (int i = 0; i < VOICE_COUNT; i++) {
        this->voices[i].envelope.envelope.active = false;
    }
    this->previous_output = 0;
    this->previous_sample = 0;
    this->next_sample = 0;
    this->delta = 0;
}

void synth_init(void) {
    if (theSynth == NULL)
        theSynth = new Synth();
}

int32_t high_freq_pwm_signal_phase0(void) {
    return theSynth->sample(0);
}

int32_t high_freq_pwm_signal_phase1(void) {
    return theSynth->sample(1);
}

int32_t high_freq_pwm_signal_phase2(void) {
    return theSynth->sample(2);
}

int32_t high_freq_pwm_signal_phase3(void) {
    return theSynth->sum_samples();
}

}