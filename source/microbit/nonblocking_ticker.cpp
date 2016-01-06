#ifndef __MICROPY_INCLUDED_MICROBIT_NONBLOCKING_TICKER_H__
#define __MICROPY_INCLUDED_MICROBIT_NONBLOCKING_TICKER_H__


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

#include "MicroBit.h"

extern "C" {

#include "nonblocking_ticker.h"
#include "device.h"

void callback_noop(void) {
    return;
}

static callback_funcptr the_callback_function = callback_noop;
static int32_t interval_ticks;

#define TheTimer NRF_TIMER0
#define TheTimer_IRQn TIMER0_IRQn
#define TheTimer_IRQHandler TIMER0_IRQHandler

/**
 * As simpler version of the RTC1 interrupt handler
 * that only supports one callback and more importantly,
 * does *not* disable interrupts.
 */
void TheTimer_IRQHandler(void)
{
    TheTimer->EVENTS_COMPARE[0] = 0;
    TheTimer->TASKS_CLEAR = 1;
    TheTimer->CC[0] = interval_ticks;
    the_callback_function();
}

/**
 * Start the timer.
 */
static void timer0_start(uint32_t interval_ms)
{
    TheTimer->POWER = 1;
    __NOP();
    interval_ticks = interval_ms*125;
    TheTimer->PRESCALER = 7; /* 125 kHz */
    TheTimer->MODE = TIMER_MODE_MODE_Timer;
    TheTimer->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
    TheTimer->INTENSET = TIMER_INTENSET_COMPARE0_Msk;

    // Use same priority as the RTC1 timer.
    NVIC_SetPriority(TheTimer_IRQn, 3);
    NVIC_ClearPendingIRQ(TheTimer_IRQn);
    NVIC_EnableIRQ(TheTimer_IRQn);

    TheTimer->TASKS_CLEAR = 1;
    TheTimer->CC[0] = interval_ticks;

    TheTimer->TASKS_START = 1;
    __NOP();
}

/** Stop the timer. */
void timer0_stop(void)
{
    NVIC_DisableIRQ(TheTimer_IRQn);
    TheTimer->INTENCLR = TIMER_INTENSET_COMPARE0_Msk;
    TheTimer->TASKS_STOP = 1;
    TheTimer->TASKS_SHUTDOWN = 1;
    TheTimer->POWER = 0;
}

void non_blocking_timer_take_over(callback_funcptr callback) {
    the_callback_function = callback;
    uBit.systemTicker.detach();
    timer0_start(FIBER_TICK_PERIOD_MS);
}

void non_blocking_timer_revert(callback_funcptr callback) {
    the_callback_function = callback_noop;
    timer0_stop();
    uBit.systemTicker.attach(callback, MICROBIT_DISPLAY_REFRESH_PERIOD);
}

}

#endif // __MICROPY_INCLUDED_MICROBIT_NONBLOCKING_TICKER_H__





