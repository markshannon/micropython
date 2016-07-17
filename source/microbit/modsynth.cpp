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

extern "C" {

#include "microbit/synthesiser.h"
#include "modmicrobit.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/objstr.h"
#include "py/mphal.h"
#include "py/nlr.h"


const mp_obj_type_t microbit_synth_frame_type = {
    { &mp_type_type },
    .name = MP_QSTR_SynthFrame,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL
};

static void framecheck_error(void) {
    nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "not a SynthFrame"));
}

static inline microbit_synth_frame_t *framecheck(mp_obj_t obj) {
    if (!MP_OBJ_IS_OBJ(obj) || ((mp_obj_base_t *)obj)->type != &microbit_synth_frame_type) {
        framecheck_error();
    }
    return (microbit_synth_frame_t *)obj;
}

static void itercheck(mp_obj_t obj) {
    mp_obj_type_t *tp = mp_obj_get_type(obj);
    if (tp == &mp_type_str || tp->iternext == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "not a suitable iterator"));
    }
}

/** WARNING; This is only safe on iterators that have passed the itercheck(iter) check. */
static inline mp_obj_t fast_iternext(mp_obj_t obj) {
    return ((mp_obj_base_t*)obj)->type->iternext(obj);
}

/* Level objects */
extern int32_t float_to_fixed(float f, uint32_t scale);

static mp_obj_t microbit_make_level(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

static mp_obj_t level_next(mp_obj_t self_in) {
    microbit_synth_level_t *self = (microbit_synth_level_t *)self_in;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i+=2) {
        self->output->data[i] = self->value;
        self->output->data[i+1] = self->value;
    }
    return self->output;
}

static mp_obj_t make_output(mp_obj_t input);

const mp_obj_type_t microbit_synth_level_type = {
    { &mp_type_type },
    .name = MP_QSTR_Level,
    .print = NULL,
    .make_new = microbit_make_level,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = level_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

static mp_obj_t microbit_make_level(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    (void)args;
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_obj_t level_in = args[0];
    microbit_synth_level_t *result = m_new_obj(microbit_synth_level_t);
    result->base.type = &microbit_synth_level_type;
    result->value = float_to_fixed(mp_obj_get_float(level_in), 14);
    result->output = m_new_obj(microbit_synth_frame_t);
    result->output->base.type = &microbit_synth_frame_type;
    printf("Fixed point value: %d\r\n", result->value);
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        result->output->data[i] = result->value;
    }
    return result;
}

/* Merge objects */

static mp_obj_t microbit_make_merge(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

static mp_obj_t merge_next(mp_obj_t self_in);

const mp_obj_type_t microbit_synth_add_type = {
    { &mp_type_type },
    .name = MP_QSTR_Add,
    .print = NULL,
    .make_new = microbit_make_merge,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = make_output,
    .iternext = merge_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ NULL,
};

const mp_obj_type_t microbit_synth_multiply_type = {
    { &mp_type_type },
    .name = MP_QSTR_Multiply,
    .print = NULL,
    .make_new = microbit_make_merge,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = make_output,
    .iternext = merge_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ NULL,
};

#define SCALE_SHIFT 14
#define SCALED_ONE ((1<<SCALE_SHIFT)-1)

static mp_obj_t merge_next(mp_obj_t self_in) {
    microbit_synth_merge_t *self = (microbit_synth_merge_t *)self_in;
    mp_obj_t lobj = fast_iternext(self->left);
    microbit_synth_frame_t *lframe = framecheck(lobj);
    mp_obj_t robj = fast_iternext(self->right);
    microbit_synth_frame_t *rframe = framecheck(robj);
    if (self->base.type == &microbit_synth_multiply_type) {
        for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
            lframe->data[i] = (lframe->data[i] * rframe->data[i])>>SCALE_SHIFT;
        }
    } else {
        for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
            lframe->data[i] += rframe->data[i];
        }
    }
    return lobj;
}

static mp_obj_t microbit_make_merge(const mp_obj_type_t *type, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    mp_obj_t left = args[0];
    mp_obj_t right = args[1];
    itercheck(left);
    itercheck(right);
    microbit_synth_merge_t *result = m_new_obj(microbit_synth_merge_t);
    result->base.type = type;
    result->left = left;
    result->right = right;
    return result;
}

/* Waveform objects */

wavefunc_t waveform_functions[] = {
    noise,
    sine_wave,
    sawtooth_wave,
    triangle_wave,
    square_wave,
    rectangle1_wave,
    rectangle2_wave,
    rectangle3_wave,
};

static mp_obj_t waveform_next(mp_obj_t self_in);

static mp_obj_t microbit_make_waveform(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

const mp_obj_type_t microbit_synth_waveform_type = {
    { &mp_type_type },
    .name = MP_QSTR_Waveform,
    .print = NULL,
    .make_new = microbit_make_waveform,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = make_output,
    .iternext = waveform_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ NULL,
};

static mp_obj_t microbit_make_waveform(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    (void)args;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    mp_obj_t kind_in = args[0];
    mp_obj_t input = args[1];
    microbit_synth_waveform_t *result = m_new_obj(microbit_synth_waveform_t);
    result->base.type = &microbit_synth_waveform_type;
    mp_int_t kind = mp_obj_get_int(kind_in);
    if (kind < 0 || kind >= (signed)MP_ARRAY_SIZE(waveform_functions)) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "not a kind of waveform"));
    }
    result->waveform = waveform_functions[kind];
    result->phase = 0;
    itercheck(input);
    result->input = input;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_make_waveform_obj, microbit_make_waveform);

static mp_obj_t waveform_next(mp_obj_t self_in) {
    microbit_synth_waveform_t *self = (microbit_synth_waveform_t *)self_in;
    mp_obj_t obj = fast_iternext(self->input);
    microbit_synth_frame_t *frame = framecheck(obj);
    self->phase = self->waveform(self->phase, frame);
    return obj;
}

/************ Wave functions **********/

int16_t sine_wave(int16_t phase_in, microbit_synth_frame_t *frame) {
    int32_t phase = phase_in;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += frame->data[i];
        /* magnitude = 2x - x**2 */
        int nsign = 1-((phase>>14)&2);
        int32_t x = ((uint32_t)phase<<17)>>17;
        int32_t x2 = (x*x)>>SCALE_SHIFT;
        int32_t magnitude = x + x - x2;
        frame->data[i] = nsign*magnitude;
    }
    return phase;
}

int16_t square_wave(int16_t phase_in, microbit_synth_frame_t *frame) {
    int32_t phase = phase_in;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += frame->data[i];
        int32_t sign = ((phase>>14)&2)-1;
        frame->data[i] = sign*SCALED_ONE;
    }
    return phase;
}

static inline int16_t rectangle(int16_t phase_in, microbit_synth_frame_t *frame, int32_t eighths) {
    int32_t phase = phase_in;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += frame->data[i];
        int32_t eighth = ((phase>>13)&7);
        if (eighth < eighths) {
            frame->data[i] = SCALED_ONE;
        } else {
            frame->data[i] = -SCALED_ONE;
        }
    }
    return phase;
}

int16_t rectangle1_wave(int16_t phase, microbit_synth_frame_t *frame) {
    return rectangle(phase, frame, 1);
}

int16_t rectangle2_wave(int16_t phase, microbit_synth_frame_t *frame) {
    return rectangle(phase, frame, 2);
}

int16_t rectangle3_wave(int16_t phase, microbit_synth_frame_t *frame) {
    return rectangle(phase, frame, 3);
}

int16_t sawtooth_wave(int16_t phase_in, microbit_synth_frame_t *frame) {
    int32_t phase = phase_in;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += frame->data[i];
        frame->data[i] = phase>>(15-SCALE_SHIFT);
    }
    return phase;
}

int16_t triangle_wave(int16_t phase, microbit_synth_frame_t *frame) {
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += frame->data[i];
        int32_t sign = ((phase>>14)&2)-1;
        frame->data[i] = (sign*phase)-SCALED_ONE;
    }
    return phase;
}

static uint32_t xorDelta128(void) {
    static uint32_t x = 3237912535U;
    static uint32_t y = 3758262409U;
    static uint32_t z = 1248895752U;
    static uint32_t w = 1877947559U;
    uint32_t t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    t = w ^ (w >> 19) ^ t ^ (t >> 8);
    w = t;
    return t;
}

int16_t noise(int16_t phase __attribute__ ((unused)), microbit_synth_frame_t *frame) {
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        frame->data[i] = xorDelta128()>>12;
    }
    return phase;
}

static mp_obj_t microbit_lowpass_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

static mp_obj_t lowpass_next(mp_obj_t self_in) {
    microbit_synth_lowpass_t * self = (microbit_synth_lowpass_t *)self_in;
    mp_obj_t obj = fast_iternext(self->input);
    framecheck(obj);
    microbit_synth_frame_t *frame = (microbit_synth_frame_t *)obj;
    int32_t z1 = self->z1;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        int32_t z0 = frame->data[i] + ((self->a*z1)>>SCALE_SHIFT);
        frame->data[i] = (self->b*(z0+z1))>>SCALE_SHIFT;
        z1 = z0;
    }
    self->z1 = z1;
    return obj;
}

static inline uint8_t offset_and_clamp(int16_t in) {
    int32_t result = (in>>(SCALE_SHIFT-7)) + 128;
    if (((uint32_t)result) > 255) {
        if (result > 0) {
            return 0;
        } else {
            return 255;
        }
    }
    return result;
}

static mp_obj_t output_next(mp_obj_t self_in) {
    microbit_synth_output_t * self = (microbit_synth_output_t *)self_in;
    // No need for framecheck as the input must be a synth element.
    microbit_synth_frame_t *frame = (microbit_synth_frame_t *)fast_iternext(self->input);
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        self->output->data[i] = offset_and_clamp(frame->data[i]);
    }
    return self->output;
}

const mp_obj_type_t microbit_synth_output_type = {
    { &mp_type_type },
    .name = MP_QSTR_Output,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = output_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ NULL,
};


/** Make an audio-frame iterator output from a
 * synth frame iterator
 */
static mp_obj_t make_output(mp_obj_t input) {
    microbit_synth_output_t *result = m_new_obj(microbit_synth_output_t);
    result->base.type = &microbit_synth_output_type;
    result->input = input;
    result->output = new_microbit_audio_frame();
    return result;
}

const mp_obj_type_t microbit_synth_lowpass_type = {
    { &mp_type_type },
    .name = MP_QSTR_LowPass,
    .print = NULL,
    .make_new = microbit_lowpass_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = make_output,
    .iternext = lowpass_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ NULL,
};

struct LowPassCoefficents {
    uint16_t frequency;
    int16_t a;
    int16_t b;
};

#define SCALED_FLOAT(f) ((int16_t)((1<<SCALE_SHIFT)*f))

const LowPassCoefficents lowpass_filter_coefficients[] = {
    { 250,  SCALED_FLOAT(0.9063471690191471), SCALED_FLOAT(0.04682641549042643) },
    { 500,  SCALED_FLOAT(0.8206787908286604), SCALED_FLOAT(0.08966060458566984) },
    { 1000, SCALED_FLOAT(0.6681786379192989), SCALED_FLOAT(0.16591068104035056) },
    { 2000, SCALED_FLOAT(0.4142135623730951), SCALED_FLOAT(0.29289321881345237) },
    { 4000, SCALED_FLOAT(0), SCALED_FLOAT(0.5) },
    { 8000, SCALED_FLOAT(-1), SCALED_FLOAT(1)  },
    { 0, 0, 0 }
};

static mp_obj_t microbit_lowpass_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    int32_t freq = mp_obj_get_int(args[0]);
    mp_obj_t input = args[1];
    const LowPassCoefficents * coefficients = &lowpass_filter_coefficients[0];
    while (coefficients->frequency != 0) {
        if (coefficients->frequency == freq) {
            microbit_synth_lowpass_t *result = m_new_obj(microbit_synth_lowpass_t);
            result->base.type = &microbit_synth_lowpass_type;
            itercheck(input);
            result->input = input;
            result->a = coefficients->a;
            result->b = coefficients->b;
            result->z1 = 0;
            return result;
        }
        coefficients++;
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                                       "Unsupported frequency for low-pass filter"));
}

/* Envelope objects */

/** Functions embodying the 5 stages of an ADSR envelope. One each for each of A,D,S and R. The 5th for quiet. */

#define ADSR_SCALE_SHIFT 16

int16_t adsr_quiet(int16_t signal __attribute__ ((unused)), microbit_synth_envelope_t *env __attribute__ ((unused))) {
    return 0;
}

int16_t adsr_release(int16_t signal, microbit_synth_envelope_t *env) {
    int32_t new_level =  env->level - env->release_rate;
    if (new_level <= 0) {
        env->level = 0;
        env->state = adsr_quiet;
        return 0;
    }
    env->level = new_level;
    return (signal*new_level)>>ADSR_SCALE_SHIFT;
}

int16_t adsr_sustain(int16_t signal, microbit_synth_envelope_t *env) {
    return (signal*env->level)>>ADSR_SCALE_SHIFT;
}

int16_t adsr_decay(int16_t signal, microbit_synth_envelope_t *env) {
    int32_t new_level =  env->level- env->decay_rate;
    if (new_level < env->sustain_level) {
        new_level = env->sustain_level;
        env->state = adsr_sustain;
    }
    env->level = new_level;
    return (signal*new_level)>>ADSR_SCALE_SHIFT;
}

int16_t adsr_attack(int16_t signal, microbit_synth_envelope_t *env) {
    int32_t new_level =  env->level + env->attack_rate;
    if (new_level >= (1<<16)) {
        new_level = (1<<16)-1;
        env->state = adsr_decay;
    }
    env->level = new_level;
    return (signal*new_level)>>ADSR_SCALE_SHIFT;
}

static mp_obj_t microbit_envelope_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

static mp_obj_t envelope_next(mp_obj_t self_in) {
    microbit_synth_envelope_t * self = (microbit_synth_envelope_t *)self_in;
    mp_obj_t obj = fast_iternext(self->input);
    framecheck(obj);
    microbit_synth_frame_t *frame = (microbit_synth_frame_t *)obj;
    for (unsigned i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        frame->data[i] = self->state(frame->data[i], self);
    }
    return obj;
}

static mp_obj_t microbit_envelope_press(mp_obj_t self_in) {
    microbit_synth_envelope_t * self = (microbit_synth_envelope_t *)self_in;
    self->state = adsr_attack;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_envelope_press_obj, microbit_envelope_press);

static mp_obj_t microbit_envelope_release(mp_obj_t self_in) {
    microbit_synth_envelope_t * self = (microbit_synth_envelope_t *)self_in;
    self->state = adsr_release;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_envelope_release_obj, microbit_envelope_release);

static const mp_map_elem_t microbit_envelope_locals_dict_table[] = {
   // { MP_OBJ_NEW_QSTR(MP_QSTR_set_attack), (mp_obj_t)&microbit_envelope_set_attack_obj },
   // { MP_OBJ_NEW_QSTR(MP_QSTR_set_decay), (mp_obj_t)&microbit_envelope_set_decay_obj },
   // { MP_OBJ_NEW_QSTR(MP_QSTR_set_sustain), (mp_obj_t)&microbit_envelope_set_sustain_obj },
   // { MP_OBJ_NEW_QSTR(MP_QSTR_set_release), (mp_obj_t)&microbit_envelope_set_release_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_press), (mp_obj_t)&microbit_envelope_press_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_release), (mp_obj_t)&microbit_envelope_release_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_envelope_locals_dict, microbit_envelope_locals_dict_table);

const mp_obj_type_t microbit_synth_envelope_type = {
    { &mp_type_type },
    .name = MP_QSTR_Envelope,
    .print = NULL,
    .make_new = microbit_envelope_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = make_output,
    .iternext = envelope_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    /* .locals_dict = */ (mp_obj_dict_t*)&microbit_envelope_locals_dict,
};

static int32_t float_obj_to_fixed(mp_obj_t value_in, uint32_t scale, float default_val) {
    float f;
    if (value_in == NULL)
        f = default_val;
    else
        f = mp_obj_get_float(value_in);
    return float_to_fixed(f, scale);
}


static mp_obj_t microbit_envelope_make_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args_in) {

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_input,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_attack,   MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_decay,    MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sustain,  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_release,  MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_loop,     MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    // Parse the args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    (void)type_in;
    microbit_synth_envelope_t *result = m_new_obj(microbit_synth_envelope_t);
    result->base.type = &microbit_synth_envelope_type;
    itercheck(args[0].u_obj);
    result->input = args[0].u_obj;
    result->attack_rate = float_obj_to_fixed(args[1].u_obj, 16, 0.001);
    result->decay_rate = float_obj_to_fixed(args[2].u_obj, 16, 0.001);
    result->sustain_level = float_obj_to_fixed(args[3].u_obj, 16, 0.7);
    result->release_rate = float_obj_to_fixed(args[4].u_obj, 16, 0.0005);
    result->state = adsr_quiet;
    return result;
}

static const mp_map_elem_t globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_synth) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_NOISE), MP_OBJ_NEW_SMALL_INT(0) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SINE), MP_OBJ_NEW_SMALL_INT(1) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SAW), MP_OBJ_NEW_SMALL_INT(2) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TRIANGLE), MP_OBJ_NEW_SMALL_INT(3) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SQUARE), MP_OBJ_NEW_SMALL_INT(4) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RECTANGLE1), MP_OBJ_NEW_SMALL_INT(5) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RECTANGLE2), MP_OBJ_NEW_SMALL_INT(6) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RECTANGLE3), MP_OBJ_NEW_SMALL_INT(7) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_Add), (mp_obj_t)&microbit_synth_add_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Multiply), (mp_obj_t)&microbit_synth_multiply_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Envelope), (mp_obj_t)&microbit_synth_envelope_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_LowPass), (mp_obj_t)&microbit_synth_lowpass_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Level), (mp_obj_t)&microbit_synth_level_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Waveform), (mp_obj_t)&microbit_synth_waveform_type},
};

static MP_DEFINE_CONST_DICT(module_globals, globals_table);

const mp_obj_module_t synth_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_synth,
    .globals = (mp_obj_dict_t*)&module_globals,
};


}


