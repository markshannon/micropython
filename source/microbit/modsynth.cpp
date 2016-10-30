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

#include "modaudio.h"

typedef uint8_t (*wavegen_funcptr)(uint16_t phase);

typedef struct _wavegen_t {
    wavegen_funcptr func;
    qstr name;
} wavegen_t;

uint8_t pseudo_sine_func(uint16_t phase) {
    /* Approximate sine function over range 0 to PI with (x - x**2) over range 0 to 1 */
    int nsign = 1-((phase>>14)&2);
    int32_t x = phase&0x7fff; // 2**16 > x >= 0
    int32_t x2 = (x*x)>>15;
    /* Scale to fit */
    int32_t magnitude = ((x-x2)*127)>>13;
    return nsign*magnitude + 128;
}

uint8_t square_func(uint16_t phase) {
    if (phase >= (1<<15))
        return 254;
    else
        return 2;
}

uint8_t sawtooth_func(uint16_t phase) {
    return phase>>8;
}

static inline uint8_t to_uint8_saturating(int32_t in) {
    in += 128;
    int32_t overflow = in >> 8;
    if (overflow) {
        return overflow < 0 ? 0 : 255;
    }
    return in;
}

/* Phase shift per sample for 2**32 per cycle and 8kHz sampe rate. */
#define PHASE_MULTIPLIER 536871

typedef struct _microbit_oscillator_obj_t {
    mp_obj_base_t base;
    uint16_t phase;
    wavegen_funcptr waveform;
    uint16_t phase_delta;
    microbit_audio_frame_obj_t *output;
} microbit_oscillator_obj_t;

static mp_obj_t oscillator_next(mp_obj_t self_in) {
    microbit_oscillator_obj_t *self = (microbit_oscillator_obj_t *)self_in;
    uint16_t phase = self->phase;
    uint16_t delta = self->phase_delta;
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        phase += delta;
        self->output->data[i] = self->waveform(phase);
    }
    self->phase = phase;
    return self->output;
}

static const wavegen_t waveforms[] = {
    {
        .func = square_func,
        .name = MP_QSTR_square,
    },
    {
        .func = pseudo_sine_func,
        .name = MP_QSTR_sine,
    },
    {
        .func = sawtooth_func,
        .name = MP_QSTR_sawtooth,
    },
};

static wavegen_funcptr wave_from_name(mp_obj_t name_in) {
    qstr name = mp_obj_str_get_qstr(name_in);
    for (uint i = 0; i < MP_ARRAY_SIZE(waveforms); i++) {
        if (name == waveforms[i].name) {
            return waveforms[i].func;
        }
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "invalid waveform"));
}

static mp_obj_t new_oscillator(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

static void check_frequency(int32_t freq) {
    if (freq < 1 || freq > 4000) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "illegal frequency"));
    }
}

mp_obj_t oscillator_set_frequency(mp_obj_t self_in, mp_obj_t freq_in) {
    microbit_oscillator_obj_t *self = (microbit_oscillator_obj_t*)self_in;
    int32_t freq = mp_obj_get_int(freq_in);
    check_frequency(freq);
    self->phase_delta = (freq*PHASE_MULTIPLIER)>>16;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(oscillator_set_frequency_obj, oscillator_set_frequency);

static const mp_map_elem_t oscillator_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_frequency), (mp_obj_t)&oscillator_set_frequency_obj },
};
static MP_DEFINE_CONST_DICT(oscillator_dict, oscillator_table);

const mp_obj_type_t microbit_oscillator_type = {
    { &mp_type_type },
    .name = MP_QSTR_Oscillator,
    .print = NULL,
    .make_new = new_oscillator,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = oscillator_next,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&oscillator_dict,
};

static mp_obj_t make_oscillator(wavegen_funcptr wave, int freq) {
    microbit_oscillator_obj_t *self = m_new_obj(microbit_oscillator_obj_t);
    self->base.type = &microbit_oscillator_type;
    self->phase = 0;
    self->waveform = wave;
    self->phase_delta = (freq*PHASE_MULTIPLIER)>>16;
    self->output = new_microbit_audio_frame();
    return self;
}


static mp_obj_t new_oscillator(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    wavegen_funcptr wave = wave_from_name(args[0]);
    int freq;
    if (n_args == 2) {
        freq = mp_obj_get_int(args[1]);
    } else {
        freq = 1000;
    }
    return make_oscillator(wave, freq);
}

typedef struct _microbit_phase_shifter_obj_t {
    mp_obj_base_t base;
    mp_obj_t input;
    uint16_t sample;
    uint16_t sample_delta;
    uint16_t base_frequency;
    microbit_audio_frame_obj_t *input_frame;
    microbit_audio_frame_obj_t *output;
} microbit_phase_shifter_obj_t;

static microbit_audio_frame_obj_t *next_frame(mp_obj_t iter) {
    mp_obj_t next = mp_iternext(iter);
    if (mp_obj_get_type(next) != &microbit_audio_frame_type)
         nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "not an audio frame"));
    return (microbit_audio_frame_obj_t *)next;
}

static mp_obj_t phase_shifter_next(mp_obj_t self_in) {
    microbit_phase_shifter_obj_t *self = (microbit_phase_shifter_obj_t *)self_in;
    uint16_t sample = self->sample;
    uint16_t delta = self->sample_delta;
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        uint8_t low = self->input_frame->data[sample>>8];
        if ((sample>>8) == 31) {
            self->input_frame = next_frame(self->input);
        }
        uint8_t high = self->input_frame->data[(sample>>8)+1];
        uint16_t next_sample = sample + delta;
        if (((sample>>8) < 31) && (next_sample>>13) != (sample>>13)) {
            self->input_frame = next_frame(self->input);
        }
        self->output->data[i] = (low*(256-(sample&255)) + high*(sample&255))>>8;
    }
    return self->output;
}

static mp_obj_t new_phase_shifter(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

const mp_obj_type_t microbit_phase_shifter_type = {
    { &mp_type_type },
    .name = MP_QSTR_PhaseShifter,
    .print = NULL,
    .make_new = new_phase_shifter,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = phase_shifter_next,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

static mp_obj_t new_phase_shifter(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 2, 2, false);
    microbit_phase_shifter_obj_t *self = m_new_obj(microbit_phase_shifter_obj_t);
    self->base.type = &microbit_phase_shifter_type;
    self->input = mp_getiter(args[0]);
    int32_t freq = mp_obj_get_int(args[1]);
    check_frequency(freq);
    self->base_frequency = freq;
    return self;
}


typedef struct _microbit_adder_obj_t {
    mp_obj_base_t base;
    mp_obj_t left;
    mp_obj_t right;
    uint16_t left_scale;
    uint16_t right_scale;
    microbit_audio_frame_obj_t *output;
} microbit_adder_obj_t;

#define ADDER_SCALE_FACTOR 12

static mp_obj_t adder_next(mp_obj_t self_in) {
    microbit_adder_obj_t *self = (microbit_adder_obj_t *)self_in;
    microbit_audio_frame_obj_t *lframe = next_frame(self->left);
    microbit_audio_frame_obj_t *rframe = next_frame(self->right);
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        int32_t val = (((int32_t)lframe->data[i])-128)*self->left_scale + (((int32_t)rframe->data[i])-128)*self->right_scale;
        val = val>>ADDER_SCALE_FACTOR;
        self->output->data[i] = to_uint8_saturating(val);
    }
    return self->output;
}

static mp_obj_t new_adder(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

const mp_obj_type_t microbit_adder_type = {
    { &mp_type_type },
    .name = MP_QSTR_Adder,
    .print = NULL,
    .make_new = new_adder,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = adder_next,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

extern int32_t float_to_fixed(float f, uint32_t scale);

static float float_in_range(mp_obj_t f, float low, float high) {
    float result = mp_obj_get_float(f);
    if (result < low || result > high) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "out of range"));
    }
    return result;
}

static mp_obj_t new_adder(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 4, 4, false);
    float left_level = float_in_range(args[1], 0, 2);
    float right_level = float_in_range(args[3], 0, 2);
    microbit_adder_obj_t *self = m_new_obj(microbit_adder_obj_t);
    self->base.type = &microbit_adder_type;
    self->left = args[0];
    self->left_scale = float_to_fixed(left_level, ADDER_SCALE_FACTOR);
    self->right = args[2];
    self->right_scale = float_to_fixed(right_level, ADDER_SCALE_FACTOR);
    self->output = new_microbit_audio_frame();
    return self;
}

typedef uint16_t (*envelope_func)(struct _microbit_envelope_obj_t * env);

typedef struct _microbit_envelope_obj_t {
    mp_obj_base_t base;
    envelope_func state;
    uint16_t volume;
    uint16_t attack_rate;
    uint16_t decay_rate;
    uint16_t release_rate;
    uint16_t sustain_level;
    mp_obj_t input;
    microbit_audio_frame_obj_t *output;
} microbit_envelope_obj_t;


#define ENVELOPE_SCALE_FACTOR 15

static inline uint8_t scale(uint8_t signal, int32_t scale) {
    int32_t val = (((((int32_t)signal)-128)*scale)>>ENVELOPE_SCALE_FACTOR)+128;
    uint32_t uval = val;
    if (uval > 255) {
        if (val < 0)
            return 0;
        else
            return 255;
    }
    return val;
}

uint16_t adsr_quiet(microbit_envelope_obj_t *env) {
    (void)env;
    return 0;
}

uint16_t adsr_release(microbit_envelope_obj_t *env) {
    int32_t new_volume = env->volume - env->release_rate;
    if (new_volume < 0) {
        new_volume = 0;
        env->state = adsr_quiet;
    }
    return new_volume;
}

uint16_t adsr_sustain(microbit_envelope_obj_t *env) {
    return env->volume;
}

uint16_t adsr_decay(microbit_envelope_obj_t *env) {
    int32_t new_volume = env->volume - env->decay_rate;
    if (new_volume < env->sustain_level) {
        new_volume = env->sustain_level;
        env->state = adsr_sustain;
    }
    return new_volume;
}


uint16_t adsr_attack(microbit_envelope_obj_t *env) {
    int32_t new_volume = env->volume + env->attack_rate;
    if (new_volume > (1<<ENVELOPE_SCALE_FACTOR)) {
        new_volume = 1<<ENVELOPE_SCALE_FACTOR;
        env->state = adsr_decay;
    }
    return new_volume;
}

static mp_obj_t envelope_next(mp_obj_t self_in) {
    microbit_envelope_obj_t *self = (microbit_envelope_obj_t *)self_in;
    microbit_audio_frame_obj_t *frame = next_frame(self->input);
    for (int i = 0; i < AUDIO_CHUNK_SIZE; i++) {
        uint16_t new_volume = self->state(self);
        self->volume = new_volume;
        self->output->data[i] = scale(frame->data[i], new_volume);
    }
    return self->output;
}

mp_obj_t press(mp_obj_t self_in) {
    microbit_envelope_obj_t *self = (microbit_envelope_obj_t*)self_in;
    self->state = adsr_attack;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(press_obj, press);

mp_obj_t release(mp_obj_t self_in) {
    microbit_envelope_obj_t *self = (microbit_envelope_obj_t*)self_in;
    self->state = adsr_release;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(release_obj, release);


mp_obj_t envelope_set_frequency(mp_obj_t self_in, mp_obj_t freq) {
    microbit_envelope_obj_t *self = (microbit_envelope_obj_t*)self_in;
    mp_obj_t bound_method[2];
    mp_load_method(self->input, MP_QSTR_set_frequency, bound_method);
    return mp_call_function_2(bound_method[0], self->input, freq);
}
static MP_DEFINE_CONST_FUN_OBJ_2(envelope_set_frequency_obj, envelope_set_frequency);

static const mp_map_elem_t envelope_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_press), (mp_obj_t)&press_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_release), (mp_obj_t)&release_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_frequency), (mp_obj_t)&envelope_set_frequency_obj },
};
static MP_DEFINE_CONST_DICT(envelope_dict, envelope_table);

static mp_obj_t new_envelope(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

const mp_obj_type_t microbit_envelope_type = {
    { &mp_type_type },
    .name = MP_QSTR_Envelope,
    .print = NULL,
    .make_new = new_envelope,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = envelope_next,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&envelope_dict,
};

static mp_obj_t make_envelope(mp_obj_t input, float attack, float decay, float sustain, float release) {
    microbit_envelope_obj_t *self = m_new_obj(microbit_envelope_obj_t);
    self->base.type = &microbit_envelope_type;
    self->input = input;
    self->volume = 0;
    self->state = adsr_quiet;
    self->attack_rate = float_to_fixed(attack, 3);
    self->decay_rate = float_to_fixed(decay, 3);
    self->sustain_level = float_to_fixed(sustain, ENVELOPE_SCALE_FACTOR);
    self->release_rate = float_to_fixed(release, 3);
    self->output = new_microbit_audio_frame();
    return self;
}

static mp_obj_t new_envelope(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 5, 5, false);
    float attack = float_in_range(args[1], 1, 1000000.0);
    float decay = float_in_range(args[2], 0, 1000000.0);
    float sustain = float_in_range(args[3], 0, 1);
    float release = float_in_range(args[4], 0, 1000000.0);
    return make_envelope(args[0], attack, decay, sustain, release);
}


mp_obj_t microbit_synth_make_buzzer(void) {
    mp_obj_t osc = make_oscillator(square_func, 1000);
    return make_envelope(osc, 1000, 0, 1.0, 1000);
}
static MP_DEFINE_CONST_FUN_OBJ_0(make_buzzer_obj, microbit_synth_make_buzzer);


STATIC const mp_map_elem_t synth_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_synth) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Oscillator), (mp_obj_t)&microbit_oscillator_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Adder), (mp_obj_t)&microbit_adder_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Envelope), (mp_obj_t)&microbit_envelope_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_make_buzzer), (mp_obj_t)&make_buzzer_obj },
};

STATIC MP_DEFINE_CONST_DICT(synth_module_globals, synth_globals_table);

const mp_obj_module_t synth_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_synth,
    .globals = (mp_obj_dict_t*)&synth_module_globals,
};

}



