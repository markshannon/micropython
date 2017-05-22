/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Mark Shannon
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

#include "string.h"

extern "C" {

#include "microbit/modmicrobit.h"
#include "microbit/microbitcomponent.h"

#include "lib/ticker.h"
#include "py/runtime0.h"
#include "py/runtime.h"

typedef void (*microbit_event_func_ptr_t)(mp_obj_t src, mp_obj_t ev);

static void noop(mp_obj_t src, mp_obj_t ev) {
    (void)src;
    (void)ev;
}

microbit_event_func_ptr_t microbit_event = noop;
static uint32_t source_filter;

/** Events buffer */

typedef struct _event_t {
    mp_obj_t source;
    mp_obj_t kind;
} event_t;

typedef struct _events_buffer_t {
    volatile uint8_t head, tail, mask;
    bool drop_newest;
    event_t data[1];
} events_buffer_t;

/** This IS NOT safe to use outside of interrupt */
static void buffer_append(events_buffer_t *buf, mp_obj_t src, mp_obj_t kind) {
    uint8_t next_head = (buf->head+1)&buf->mask;
    if (next_head == buf->tail) {
        // Buffer full
        if (buf->drop_newest) {
            return;
        } else {
            // Make room in buffer
            buf->tail = (buf->tail+1)&buf->mask;
        }
    }
    buf->data[buf->head].source = src;
    buf->data[buf->head].kind = kind;
    buf->head = next_head;
}

static inline int buffer_empty(events_buffer_t *buf) {
    return buf->tail == buf->head;
}

/** This IS safe to use outside of interrupt -- If buffer is empty then behaviour is undefined */
static inline event_t buffer_pop(events_buffer_t *buf) {
    event_t result;
    __disable_irq();
    uint32_t tail = buf->tail;
    result = buf->data[tail];
    buf->tail = (tail + 1)&buf->mask;
    __enable_irq();
    return result;
}

events_buffer_t *make_events_buffer(int32_t minimum_size, int drop_newest) {
    uint8_t size = 1;
    if (minimum_size < 0 || minimum_size > 128) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid buffer size"));
    }
    while (size < minimum_size) {
        size <<= 1;
    }
    events_buffer_t *result = m_new_obj_var(events_buffer_t, event_t, size-1);
    result->head = 0;
    result->head = 0;
    result->mask = size-1;
    result->drop_newest = drop_newest;
    return result;
}

#define events_buffer MP_STATE_PORT(events_buffer)

static void add_to_buffer(mp_obj_t src, mp_obj_t ev) {
    microbit_component_obj_t *component = (microbit_component_obj_t *)src;
    uint32_t id = component->id;
    if (source_filter & (1<<id)) {
        buffer_append(events_buffer, component, ev);
    }
}

typedef struct _microbit_event_stream_obj_t {
    mp_obj_base_t base;
} microbit_event_stream_obj_t;

static mp_obj_t event_stream_next(mp_obj_t self_in) {
    (void)self_in;
    while (buffer_empty(events_buffer)) {
        __WFI();
    }
    event_t event = buffer_pop(events_buffer);
    return mp_obj_new_tuple(2, &event.source);
}

const mp_obj_type_t microbit_event_stream_type = {
    { &mp_type_type },
    .name = MP_QSTR_event_stream,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = event_stream_next,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

static const microbit_event_stream_obj_t the_stream = {
    .base = { &microbit_event_stream_type }
};

static void fire_tick_event(void) {
    buffer_append(events_buffer, MP_OBJ_NEW_QSTR(MP_QSTR_tick), MP_OBJ_NEW_SMALL_INT(microbit_running_time()));
}

static const mp_obj_t components[] = {
    [MICROBIT_BUTTON_A_ID] = (mp_obj_t)&microbit_button_a_obj,
    [MICROBIT_BUTTON_B_ID] = (mp_obj_t)&microbit_button_b_obj,
};

static uint32_t filter_from_objects(mp_uint_t n_args, const mp_obj_t *pos_args) {
    uint32_t filter = 0;
    for (uint32_t i; i < n_args; i++) {
        mp_obj_t obj = pos_args[i];
        uint32_t id = ((microbit_component_obj_t *)obj)->id;
        if (id > MP_ARRAY_SIZE(components) || components[id] != pos_args[i]) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid event source"));
        }
        filter |= (1<<id);
    }
    return filter;
}

STATIC mp_obj_t microbit_event_stream(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_tick, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_queue, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 4} },
        { MP_QSTR_drop_newest, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(0, NULL, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    events_buffer = make_events_buffer(args[1].u_int, args[2].u_bool);
    if (args[0].u_int > 0) {
        microbit_ticker_interval(args[0].u_int, fire_tick_event);
    }
    source_filter = filter_from_objects(n_args, pos_args);
    microbit_event = add_to_buffer;
    return (mp_obj_t)&the_stream;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_event_stream_obj, 0, microbit_event_stream);

STATIC const mp_map_elem_t microbit_events_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_event_stream), (mp_obj_t)&microbit_event_stream_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_events_dict, microbit_events_dict_table);

const mp_obj_module_t events_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_events,
    .globals = (mp_obj_dict_t*)&microbit_events_dict,
};

}
