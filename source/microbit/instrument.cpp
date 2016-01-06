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


#include "microbit/instrument.h"
#include "microbit/synthesiser.h"
#include <stdio.h>
#include <cstddef>

extern "C" {

union _i2f {
    int32_t bits;
    float value;
};

/* Convert a float in the range -1.0 to +1.0 to a fixed-point number */
int32_t float_to_fixed(float f, uint32_t scale) {
    union _i2f x;
    x.value = f;
    int32_t sign = 1-((x.bits>>30)&2);
    /* Subtract 127 from exponent for IEEE-754 and 23 for mantissa scaling */
    int32_t exponent = ((x.bits>>23)&255)-150;
    /* Mantissa scaled by 2**23, including implicit 1 */
    int32_t mantissa = (1<<23) | ((x.bits)&((1<<23)-1));
    int32_t shift = scale+exponent;
    int32_t result;
    if (shift > 0) {
        result = sign*(mantissa<<shift);
    } else if (shift < -31) {
        result = 0;
    } else {
        result = sign*(mantissa>>(-shift));
    }
    // printf("Float %f: %d %d %x (scale %d) => %d\n", f, sign, exponent, mantissa, scale, result);
    return result;
}

// Not performance critical so, use simple version
float fixed_to_float(int32_t f) {
    return ((float)f)/(1<<SCALE_SHIFT);
}

static const uint32_t X = 3237912535U;
static const uint32_t Y = 3758262409U;
static const uint32_t Z = 1248895752U;
static const uint32_t W = 1877947559U;

static inline uint32_t milliSecondsToTicks(uint32_t ms) {
    //ticks = seconds * frequency = ms/1000 * freq = ms*freq/125/8
    return (ms * (SAMPLING_FREQUENCY/125))>>3;
}
static inline uint32_t ticksToMilliseconds(uint32_t ticks) {
    return (ticks * 1000)>>14;
}


void Envelope::setADSR(uint32_t attack_ms, uint32_t decay_ms, float sustain, uint32_t release_ms, EnvelopeSettings *settings) {
    //Scale sustain
    settings->sustain_level = float_to_fixed(sustain, DOUBLE_SHIFT);
    int32_t ticks = 1+milliSecondsToTicks(attack_ms);
    settings->attack_rate = (1<<DOUBLE_SHIFT)/ticks;
    ticks = 1+milliSecondsToTicks(decay_ms);
    settings->decay_rate = ((1<<DOUBLE_SHIFT)-settings->sustain_level)/ticks;
    ticks = 1+milliSecondsToTicks(release_ms);
    settings->release_rate = settings->sustain_level/ticks;
}

int32_t Envelope::getAttack(EnvelopeSettings *settings) {
     int32_t ticks = (1<<DOUBLE_SHIFT)/settings->attack_rate;
     return ticksToMilliseconds(ticks-1);
}

int32_t Envelope::getDecay(EnvelopeSettings *settings) {
     int32_t ticks = ((1<<DOUBLE_SHIFT)-settings->sustain_level)/settings->decay_rate;
     return ticksToMilliseconds(ticks-1);
}

float Envelope::getSustain(EnvelopeSettings *settings) {
    return ((float)settings->sustain_level)/(1<<DOUBLE_SHIFT);
}

int32_t Envelope::getRelease(EnvelopeSettings *settings) {
     int32_t ticks = settings->sustain_level/settings->release_rate;
     return ticksToMilliseconds(ticks-1);
}

void Envelope::initialiseFrom(EnvelopeSettings *settings) {
    this->active = false;
    this->level = 0;
    this->settings = *settings;
}

}