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

#include "stddef.h"
#include "lib/ticker.h"
#include "nrf_gpio.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "PinNames.h"

#define PWM_TICKER_INDEX 2

// Default period of 20ms
#define DEFAULT_PERIOD ((20*1000)/MICROSECONDS_PER_TICK)

typedef struct _pwm_event {
    uint16_t duty_cycle;
    uint8_t pin;
    uint8_t turn_on;
} pwm_event;

typedef struct _pwm_events {
    uint8_t count;
    uint16_t period;
    uint32_t all_pins;
    pwm_event events[1];
} pwm_events;

static const pwm_events OFF_EVENTS = {
    .count = 1,
    .period = DEFAULT_PERIOD,
    .all_pins = 0,
    .events = {
        {
            .duty_cycle = 1024,
            .pin = 31,
            .turn_on = 0
        }
    }
};

static const pwm_events *active_events = &OFF_EVENTS;
static const pwm_events *pending_events = NULL;
static uint8_t next_event = 0;

static inline int32_t pwm_get_period_ticks(void) {
    const pwm_events *tmp = pending_events;
    if (tmp == NULL)
        tmp = active_events;
    return tmp->period;
}

#if 0
void pwm_dump_events(const pwm_events *events) {
    printf("Count %d, period %d, all pins %d\r\n", events->count, events->period, events->all_pins);
    for (uint32_t i = 0; i < events->count; i++) {
        const pwm_event *event = &events->events[i];
        printf("Event. pin: %d, duty cycle: %d, turn_on: %d\r\n",
               event->pin, event->duty_cycle, event->turn_on);
    }
}

void pwm_dump_state(void) {
    while(pending_events);
    pwm_dump_events(active_events);
}
#endif

static const pwm_events *swap_pending(const pwm_events *in) {
    __disable_irq();
    const pwm_events *result = pending_events;
    pending_events = in;
    __enable_irq();
    return result;
}

static pwm_events *copy_events(const pwm_events *orig, uint32_t count) {
    pwm_events *events = m_malloc(sizeof(pwm_events) + (count-1)*sizeof(pwm_event));
    events->count = count;
    uint32_t copy = count > orig->count ? orig->count : count;
    for (uint32_t i = 0; i < copy; i++) {
        events->events[i] = orig->events[i];
    }
    return events;
}

static int find_pin_in_events(const pwm_events *events, uint32_t pin) {
    for (int i = 0; i < events->count; i++) {
        if (events->events[i].pin == pin)
            return i;
    }
    return -1;
}

static void sort_events(pwm_events *events) {
    // Insertion sort
    for (int32_t i = 1; i < events->count; i++) {
        pwm_event x = events->events[i];
        int32_t j;
        for (j = i - 1; j >= 0 && events->events[j].duty_cycle > x.duty_cycle; j--) {
            events->events[j+1] = events->events[j];
        }
        events->events[j+1] = x;
    }
}

int32_t pwm_callback(void) {
    int32_t tdiff;
    const pwm_events *events = active_events;
    const pwm_event *event = &events->events[next_event];
    int32_t tnow = (event->duty_cycle*events->period)>>10;
    do {
        if (event->turn_on) {
            nrf_gpio_pin_set(event->pin);
            next_event++;
        } else {
            nrf_gpio_pins_clear(events->all_pins);
            next_event = 0;
            tnow = 0;
            if (pending_events) {
                events = active_events = pending_events;
                pending_events = NULL;
            }
        }
        event = &events->events[next_event];
        tdiff = ((event->duty_cycle*events->period)>>10) - tnow;
    } while (tdiff == 0);
    return tdiff;
}

void pwm_start(void) {
    set_ticker_callback(PWM_TICKER_INDEX, pwm_callback, 120);
}

void pwm_stop(void) {
    clear_ticker_callback(PWM_TICKER_INDEX);
}

static void pwm_set_period_ticks(int32_t ticks) {
    const pwm_events *old_events = swap_pending(NULL);
    if (old_events == NULL) {
        old_events = active_events;
    }
    pwm_events *events = copy_events(old_events, old_events->count);
    events->all_pins = old_events->all_pins;
    events->period = ticks;
    pending_events = events;
}

int pwm_set_period_us(int32_t us) {
    if ((us < 256) ||
        (us > 1000000)) {
        return -1;
    }
    pwm_set_period_ticks(us/MICROSECONDS_PER_TICK);
    return 0;
}

int32_t pwm_get_period_us(void) {
    return pwm_get_period_ticks()*MICROSECONDS_PER_TICK;
}

void pwm_set_duty_cycle(int32_t pin, uint32_t value) {
    if (value >= (1<<10)) {
        value = (1<<10)-1;
    }
    const pwm_events *old_events = swap_pending(NULL);
    if (old_events == NULL) {
        old_events = active_events;
    }
    if (((1<<pin)&old_events->all_pins) == 0) {
         nrf_gpio_cfg_output(pin);
    }
    int ev = find_pin_in_events(old_events, pin);
    pwm_events *events;
    if (ev < 0 && value == 0) {
        return;
    } else if (ev < 0) {
        events = copy_events(old_events, old_events->count+1);
        events->all_pins = old_events->all_pins | (1<<pin);
        events->events[old_events->count].duty_cycle = value;
        events->events[old_events->count].pin = pin;
        events->events[old_events->count].turn_on = 1;
    } else if (value == 0) {
        events = copy_events(old_events, old_events->count-1);
        events->all_pins = old_events->all_pins & ~(1<<pin);
        if (ev < old_events->count-1) {
            events->events[ev] = old_events->events[old_events->count-1];
        }
    } else {
        events = copy_events(old_events, old_events->count);
        events->all_pins = old_events->all_pins;
        events->events[ev].duty_cycle = value;
    }
    events->period = old_events->period;
    sort_events(events);
    pending_events = events;
    return;
}

