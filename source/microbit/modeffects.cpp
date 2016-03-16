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

#include "microbit/modmicrobit.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/gc.h"
#include "microbit/modaudio.h"

typedef struct _microbit_reverb_obj_t {
    mp_obj_base_t base;
    uint32_t bucket_count;
    int8_t *buckets;
    uint32_t index;
    uint32_t delay;
    uint32_t reflection;
    uint32_t feedback;
    mp_obj_t source;
} microbit_reverb_obj_t;


STATIC mp_obj_t reverb_next(mp_obj_t o_in) {
    _microbit_reverb_obj_t *iter = (_microbit_reverb_obj_t *)o_in;
    mp_obj_t frame_obj = mp_iternext(iter->source);
    if (frame_obj == MP_OBJ_STOP_ITERATION) {
        return MP_OBJ_STOP_ITERATION;
    }
    if (mp_obj_get_type(frame_obj) != &microbit_audio_frame_type) {
        // TO DO -- handle error properly
        return MP_OBJ_STOP_ITERATION;
    }
    microbit_audio_frame_obj_t *frame = (microbit_audio_frame_obj_t *)frame_obj;
    uint32_t index = iter->index;
    uint32_t delay = iter->index-iter->delay;
    if (delay > iter->index) {
        delay += iter->bucket_count;
    }
    for (int i = 0; i < 32; i++) {
        int32_t reverb = (iter->buckets[delay]*iter->reflection)>>10;
        iter->buckets[index] = ((reverb*iter->feedback)>>10) + frame->data[i];
        frame->data[i] += reverb;
        ++delay;
        if (delay == iter->bucket_count) {
            delay = 0;
        }
        ++index;
        if (index  == iter->bucket_count) {
            index = 0;
        }
    }
    iter->index = index;
    return frame_obj;
};


STATIC mp_obj_t microbit_reverb_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

STATIC const mp_obj_type_t microbit_reverb_type = {
    { &mp_type_type },
    .name = MP_QSTR_Reverb,
    .print = NULL,
    .make_new = microbit_reverb_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = reverb_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

STATIC mp_obj_t microbit_reverb_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_source,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_delay,      MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 80 } },
        { MP_QSTR_buckets,    MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_reflection, MP_ARG_INT, {.u_int = 100} },
        { MP_QSTR_feedback,   MP_ARG_INT, {.u_int = 0} },
    };

    (void)type_in;
    mp_arg_val_t out_args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, out_args);
    microbit_reverb_obj_t *reverb = m_new_obj(microbit_reverb_obj_t);
    reverb->base.type = &microbit_reverb_type;
    reverb->source = mp_getiter(out_args[0].u_obj);
    reverb->delay = out_args[1].u_int;
    int32_t count = out_args[2].u_int;
    reverb->bucket_count  = count;
    if (count < 0) {
        reverb->bucket_count = reverb->delay+1;
    }
    reverb->buckets = (int8_t *)gc_alloc(reverb->bucket_count, false);
    reverb->reflection = out_args[3].u_int;
    reverb->feedback = out_args[4].u_int;
    reverb->index = 0;
    return reverb;
}

STATIC const mp_map_elem_t effects_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_effects) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Reverb), (mp_obj_t)&microbit_reverb_type },
};


STATIC MP_DEFINE_CONST_DICT(effects_module_globals, effects_globals_table);

const mp_obj_module_t effects_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_effects,
    .globals = (mp_obj_dict_t*)&effects_module_globals,
};


}


