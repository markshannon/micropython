/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#ifndef __MICROPY_INCLUDED_ASYNC_H__
#define __MICROPY_INCLUDED_ASYNC_H__

#include "py/runtime.h"

extern "C" {

typedef struct _async_t async_t;

// Callback function pointer type. Returns the desired delay (in milliseconds) until the next callback.
typedef int32_t (*async_callback_func_ptr)(async_t *async, void *state);

struct _async_t {
    int32_t ticks;
    int32_t wakeup;
    mp_obj_t iterator;
    void *state;
    mp_obj_t async_exception;
    async_callback_func_ptr callback;
    async_callback_func_ptr exit;
    bool async;
    volatile bool done;
};

// Start iteration -- 'state' is whatever state the passed functions need to operate.
// Callback is called after initial_delay_ms and then repeatedly after a delay of the return value of the previous call
// exit is called whenever iteration is stopped, either explicitly via a call to async_stop or as a result of an exception being thrown.
void async_start(async_t *async, mp_obj_t iterator, void *state, async_callback_func_ptr callback, async_callback_func_ptr exit, int initial_delay_ms, bool wait);

// Get the next item in the iterable. Returns NULL if exhausted
mp_obj_t async_get_next(async_t *async);

// Stop iteration
void async_stop(async_t *async);

// Test whether iteration is still running
bool async_is_running(async_t *async);

void async_tick(void);

}

#endif // __MICROPY_INCLUDED_ASYNC_H__
