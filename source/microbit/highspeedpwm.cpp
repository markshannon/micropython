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

#include "MicroBit.h"

extern "C" {

#include "microbit/synthesiser.h"
#include "gpio_api.h"
#include "device.h"
#include "microbitimage.h"
#include "modmicrobit.h"
#include "highspeedpwm.h"


/*******************************
 * High frequency (62.5kHz) PWM
 *****************************/

#define TheTimer NRF_TIMER1
#define TheTimer_IRQn TIMER1_IRQn
#define TheTimer_IRQHandler TIMER1_IRQHandler

extern uint8_t PWM_taken[3];

extern void gpiote_init(PinName pin, uint8_t channel_number);

/** Initialize the Programmable Peripheral Interconnect peripheral.
 * Mostly copied from pwmout_api.c
 */
static void ppi_init(uint8_t pwm) {
    //using ppi channels 0,1 and 2 (only 0-7 are available)
    NRF_TIMER_Type *timer  = TheTimer;

    // Configure PPI channels to toggle pin on every timer COMPARE match
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[pwm];
    NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[pwm];
    NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[pwm];
    NRF_PPI->CH[0].EEP = (uint32_t)&timer->EVENTS_COMPARE[0];
    NRF_PPI->CH[1].EEP = (uint32_t)&timer->EVENTS_COMPARE[1];
    NRF_PPI->CH[2].EEP = (uint32_t)&timer->EVENTS_COMPARE[2];

    // Enable PPI channels.
    NRF_PPI->CHEN |= 7;
}

/* Start and stop timer 1 including workarounds for Anomaly 73 for Timer
* http://www.nordicsemi.com/eng/content/download/29490/494569/file/nRF51822-PAN%20v3.0.pdf
*/
static inline void timer_start(void) {
    NVIC_EnableIRQ(TheTimer_IRQn);
    *(uint32_t *)0x40009C0C = 1; //for Timer 1
    TheTimer->TASKS_START = 1;
}

static inline void timer_stop(void) {
    NVIC_DisableIRQ(TheTimer_IRQn);
    TheTimer->TASKS_STOP = 1;
    *(uint32_t *)0x40009C0C = 0; //for Timer 1
}

static PinName the_pin;

/* Initalize, but do not start, timer */
void high_freq_pwm_init(PinName pin) {
    the_pin = pin;
    timer_stop();
    gpiote_init(pin, 3);
    ppi_init(3);
    //pwm_signal = signal;
    NVIC_SetPriority(TheTimer_IRQn, 1);
}

static bool pwm_running = false;

void high_freq_pwm_start(void) {
    NRF_TIMER_Type *timer = TheTimer;
    TheTimer->POWER = 1;
    __NOP();
    timer_stop();
    timer->TASKS_CLEAR = 1;
    timer->MODE = TIMER_MODE_MODE_Timer;
    timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
    timer->PRESCALER = 0;
    timer->INTENSET = TIMER_INTENSET_COMPARE3_Msk;
    timer->SHORTS = 0;
    timer->CC[2] = 528;
    timer->CC[0] = 529;
    timer->CC[3] = 512;
    timer_start();
    pwm_running = true;
}

void high_freq_pwm_stop(void) {
    pwm_running = false;
    timer_stop();
    TheTimer->INTENCLR = TIMER_INTENSET_COMPARE3_Msk;
    TheTimer->TASKS_SHUTDOWN = 1;
    TheTimer->POWER = 0;
    //Call gpiote_init again to force pin low
    gpiote_init(the_pin, 3);
}

#define USE_HIGH_FREQ_PWM_TEST_SIGNAL 0

#if USE_HIGH_FREQ_PWM_TEST_SIGNAL

/* 128Hz square wave */
int32_t high_freq_pwm_test_signal(void) {
    static int32_t test_value = 0;
    test_value++;
    if ((test_value >> 8)&1)
        return -127<<8;
    return 127<<8;
}

const signal_funcptr high_freq_pwm_signals[4] = {
    &high_freq_pwm_test_signal,
    &high_freq_pwm_test_signal,
    &high_freq_pwm_test_signal,
    &high_freq_pwm_test_signal,
};

#else

const signal_funcptr high_freq_pwm_signals[4] = {
    &high_freq_pwm_signal_phase0,
    &high_freq_pwm_signal_phase1,
    &high_freq_pwm_signal_phase2,
    &high_freq_pwm_signal_phase3,
};

#endif

static inline uint32_t timer_clock(void) {
    TheTimer->TASKS_CAPTURE[2] = 1;
    uint32_t t = TheTimer->CC[2];
    return t;
}

/*Initialize to minimum value to reduce "pop" when starting */
static int32_t sample = -127;

void TheTimer_IRQHandler(void)
{
    TheTimer->EVENTS_COMPARE[3] = 0;
    int32_t next_phase = (timer_clock()>>8)+1;
    TheTimer->CC[3] = (next_phase<<8);
    TheTimer->CC[next_phase&1] = (next_phase<<8)+sample+136;
    TheTimer->CC[2] = (next_phase<<8)+8;
    int32_t next_sample = high_freq_pwm_signals[next_phase&3]()>>(SCALE_SHIFT-7);
    sample = next_sample;
}

bool high_freq_pwm_running(void) {
    return pwm_running;
}

}
