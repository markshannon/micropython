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
#include "py/runtime.h"
#include "microbit/instrument.h"
#include "microbitimage.h"

extern "C" {

struct Oscillator {
    int32_t edge;
    int32_t phase;

    static int32_t setDutyCycle(float fraction);
    static float getDutyCycle(int32_t edge);
};

struct PrngState {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
};

struct Envelope {
    EnvelopeSettings settings;
    int32_t level;
    bool active;

    static void setADSR(uint32_t attack_ms, uint32_t decay_ms, float sustain, uint32_t release_ms, EnvelopeSettings *settings);
    void initialiseFrom(EnvelopeSettings *);
    static int32_t getAttack(EnvelopeSettings *settings);
    static int32_t getDecay(EnvelopeSettings *settings);
    static float getSustain(EnvelopeSettings *settings);
    static int32_t getRelease(EnvelopeSettings *settings);

};

struct LowPass {
    int32_t a;
    int32_t b;
    int32_t z1;
};

struct FilterComponent {
    filter_funcptr action;
    initialise_funcptr init;
    union {
        LowPass lowpass;
        Envelope envelope;
        Oscillator oscillator;
        PrngState prng;
    };
};

class Instrument {
    static int32_t setDetune(float detune);
};

struct Voice {
    int32_t detune1;
    int32_t phase_delta1;
    int32_t detune2;
    int32_t phase_delta2;
    FilterComponent source1;
    FilterComponent source2;
    FilterComponent filter1;
    FilterComponent filter2;
    Balance balance;
    FilterComponent envelope;

    int32_t sample() {
        int32_t left = this->source1.action(this->phase_delta1, &this->source1);
        left = this->filter1.action(left, &this->filter1);
        int32_t right = this->source2.action(this->phase_delta2, &this->source2);
        right = this->filter2.action(right, &this->filter2);
        int32_t out = (((int32_t)balance.left_volume)*left+((int32_t)balance.right_volume)*right)>>SCALE_SHIFT;
        return this->envelope.action(out, &this->envelope);
    }

    void setInstrument(microbit_instrument_obj_t *instrument);

    void start(uint32_t mHz);

    void release(void);

};


#define VOICE_COUNT 3

class Synth {

public:

    Voice voices[VOICE_COUNT];
    Voice *voice_ordering[VOICE_COUNT];

    int32_t previous_output;
    int32_t previous_sample;
    int32_t next_sample;
    int32_t delta;
    int32_t out_level;

    Synth();

    Voice *start(microbit_instrument_obj_t *, uint32_t mHz);

    inline int32_t sample(int32_t voice_index) {
        Voice *v = &this->voices[voice_index];
        if (v->envelope.envelope.active) {
            this->next_sample += (v->sample()*out_level)>>SCALE_SHIFT;
        }
        int32_t result = this->previous_output + ((this->delta+voice_index+1)>>2);
        this->previous_output = result;
        return result;
    }

    inline int32_t sum_samples() {
        int32_t result = this->previous_output + (this->delta>>2);
        this->previous_output = result;
        int32_t sample = this->next_sample;
        // Clamp sample.
        if ((sample>>(SCALE_SHIFT-7)) < -127)
            sample = -127<<(SCALE_SHIFT-7);
        if ((sample>>(SCALE_SHIFT-7)) > 127)
            sample = 127<<(SCALE_SHIFT-7);
        this->delta = sample - this->previous_sample;
        this->previous_sample = sample;
        this->next_sample = 0;
        return result;
    }

    void reset();

};

extern Synth *theSynth;

void synth_init(void);

}

#endif // __MICROPY_INCLUDED_MICROBIT_SYNTHESISER_H__