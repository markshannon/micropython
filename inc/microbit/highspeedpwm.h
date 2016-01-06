#ifndef __MICROPY_INCLUDED_MICROBIT_HS_PWM_H__
#define __MICROPY_INCLUDED_MICROBIT_HS_PWM_H__


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

extern "C" {

#if 1

static inline void start_timer() {
    NRF_TIMER_Type *timer = NRF_TIMER0;
    timer->TASKS_STOP = 0;
    timer->POWER     = 0;
    timer->POWER     = 1;
    timer->TASKS_CLEAR = 1;

    timer->MODE = TIMER_MODE_MODE_Timer;
    timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
    timer->PRESCALER = 0;

    timer->TASKS_START = 1;
}

static inline int32_t read_timer() {
    NRF_TIMER0->TASKS_CAPTURE[0] = 1;
    return NRF_TIMER0->CC[0];
}

#endif

typedef int32_t (*signal_funcptr)(void);

//Declare signal function array here. Must be defined by client (synth).
int32_t high_freq_pwm_signal_phase0(void);
int32_t high_freq_pwm_signal_phase1(void);
int32_t high_freq_pwm_signal_phase2(void);
int32_t high_freq_pwm_signal_phase3(void);

extern const signal_funcptr high_freq_pwm_signals[4];

void high_freq_pwm_init(PinName pin);

void high_freq_pwm_start(void);
void high_freq_pwm_stop(void);
bool high_freq_pwm_running(void);

}



#endif // __MICROPY_INCLUDED_MICROBIT_HS_PWM_H__