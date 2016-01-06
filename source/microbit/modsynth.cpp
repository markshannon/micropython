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
#include "microbitmusic.h"

extern "C" {

#include "microbit/synthesiser.h"
#include "gpio_api.h"
#include "device.h"
#include "microbitimage.h"
#include "modmicrobit.h"
#include "highspeedpwm.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/objstr.h"
#include "py/mphal.h"
#include "nonblocking_ticker.h"

extern void micropython_ticker(void);

static void typecheck(const char *name, mp_obj_t obj, const mp_obj_type_t *type, const char *tname) {
    if (!mp_obj_is_subclass_fast(mp_obj_get_type(obj), type)) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
            "%s must be a %s", name, tname));
    }
}

STATIC mp_obj_t microbit_make_rectangular(mp_obj_t duty_cycle) {
    float f = mp_obj_get_float(duty_cycle);
    microbit_oscillator_obj_t *result = m_new_obj(microbit_oscillator_obj_t);
    result->base.type = &microbit_oscillator_type;
    result->name = "rectangular";
    result->image = (mp_obj_t)&microbit_const_image_rectangular_wave_obj;
    result->waveform = rectangular_wave;
    result->edge = (float_to_fixed(f, 30) - (1<<29))<<2;
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_make_rectangular_obj, microbit_make_rectangular);

/* Oscillator objects */

STATIC void microbit_oscillator_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind __attribute__ ((unused))) {
    microbit_oscillator_obj_t *self = (microbit_oscillator_obj_t*)self_in;
    const char *name = self->name;
    if (self->waveform == &rectangular_wave) {
        mp_printf(print, "rectangular_wave(%d)", self->edge);
    } else {
        mp_printf(print, "%s", name);
    }
}

const mp_obj_type_t microbit_oscillator_type = {
    { &mp_type_type },
    .name = MP_QSTR_Oscillator,
    .print = microbit_oscillator_print,
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
    .bases_tuple = (mp_obj_t)&component_base_tuple,
    /* .locals_dict = */ MP_OBJ_NULL,
};

STATIC void microbit_lowpass_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind __attribute__ ((unused))) {
    microbit_lowpass_obj_t *self = (microbit_lowpass_obj_t*)self_in;
    mp_printf(print, "LowPass(%d)", self->settings.frequency);
}

STATIC mp_obj_t microbit_lowpass_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

const mp_obj_type_t microbit_lowpass_type = {
    { &mp_type_type },
    .name = MP_QSTR_LowPass,
    .print = microbit_lowpass_print,
    .make_new = microbit_lowpass_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = (mp_obj_t)&component_base_tuple,
    /* .locals_dict = */ MP_OBJ_NULL,
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

STATIC mp_obj_t microbit_lowpass_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    int32_t freq = mp_obj_get_int(args[0]);
    const LowPassCoefficents * coefficients = &lowpass_filter_coefficients[0];
    while (coefficients->frequency != 0) {
        if (coefficients->frequency == freq) {
            microbit_lowpass_obj_t *result = m_new_obj(microbit_lowpass_obj_t);
            result->base.type = &microbit_lowpass_type;
            result->settings.frequency = freq;
            result->settings.a = coefficients->a;
            result->settings.b = coefficients->b;
            return result;
        }
        coefficients++;
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                                       "Unsupported frequency for low-pass filter"));
}

/* Envelope objects */

STATIC mp_obj_t microbit_envelope_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

STATIC void microbit_envelope_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind __attribute__ ((unused))) {
    microbit_envelope_obj_t *self = (microbit_envelope_obj_t*)self_in;
    mp_printf(print, "Envelope(%d, %d, %f, %d)",
              Envelope::getAttack(&self->settings),
              Envelope::getDecay(&self->settings),
              Envelope::getSustain(&self->settings),
              Envelope::getRelease(&self->settings) );
}

const mp_obj_type_t microbit_envelope_type = {
    { &mp_type_type },
    .name = MP_QSTR_Envelope,
    .print = microbit_envelope_print,
    .make_new = microbit_envelope_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = (mp_obj_t)&component_base_tuple,
    /* .locals_dict = */ MP_OBJ_NULL,
};

STATIC mp_obj_t microbit_envelope_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 4, 5, false);
    microbit_envelope_obj_t *result = m_new_obj(microbit_envelope_obj_t);
    result->base.type = &microbit_envelope_type;
    Envelope::setADSR(mp_obj_get_float(args[0]),
                      mp_obj_get_float(args[1]),
                      mp_obj_get_float(args[2]),
                      mp_obj_get_float(args[3]),
                      &result->settings);
    result->attack_release = n_args == 5 && mp_obj_is_true(args[4]);
    return result;
}



/* Instrument objects */


STATIC mp_obj_t microbit_instrument_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);

STATIC mp_obj_t microbit_instrument_copy(mp_obj_t self_in) {
    microbit_instrument_obj_t *result = m_new_obj(microbit_instrument_obj_t);
    memcpy(result, self_in, sizeof(microbit_instrument_obj_t));
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_instrument_copy_obj, microbit_instrument_copy);

STATIC mp_obj_t set_instrument_float_field(mp_obj_t self_in, int offset, mp_obj_t value_in) {
     microbit_instrument_obj_t *self = (microbit_instrument_obj_t *)self_in;
     int32_t value = float_to_fixed(mp_obj_get_float(value_in), SCALE_SHIFT);
     ((int32_t *)(((char *)self) + offset))[0] = value;
     return mp_const_none;
}

STATIC mp_obj_t microbit_instrument_set_detune1(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_float_field(self_in, offsetof(microbit_instrument_obj_t, detune1), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_detune1_obj, microbit_instrument_set_detune1);


STATIC mp_obj_t microbit_instrument_set_detune2(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_float_field(self_in, offsetof(microbit_instrument_obj_t, detune2), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_detune2_obj, microbit_instrument_set_detune2);


STATIC mp_obj_t microbit_instrument_set_volume1(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_float_field(self_in, offsetof(microbit_instrument_obj_t, balance.left_volume), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_volume1_obj, microbit_instrument_set_volume1);


STATIC mp_obj_t microbit_instrument_set_volume2(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_float_field(self_in, offsetof(microbit_instrument_obj_t, balance.right_volume), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_volume2_obj, microbit_instrument_set_volume2);

STATIC mp_obj_t  set_instrument_object_field(mp_obj_t self_in, int offset, mp_obj_t value_in) {
     microbit_instrument_obj_t *self = (microbit_instrument_obj_t *)self_in;
     /* TO DO -- Type check argument */
     ((mp_obj_t *)(((char *)self) + offset))[0] = value_in;
     return mp_const_none;
}

STATIC mp_obj_t microbit_instrument_set_source1(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_object_field(self_in, offsetof(microbit_instrument_obj_t, source1), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_source1_obj, microbit_instrument_set_source1);

STATIC mp_obj_t microbit_instrument_set_source2(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_object_field(self_in, offsetof(microbit_instrument_obj_t, source2), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_source2_obj, microbit_instrument_set_source2);

STATIC mp_obj_t microbit_instrument_set_filter1(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_object_field(self_in, offsetof(microbit_instrument_obj_t, filter1), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_filter1_obj, microbit_instrument_set_filter1);

STATIC mp_obj_t microbit_instrument_set_filter2(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_object_field(self_in, offsetof(microbit_instrument_obj_t, filter2), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_filter2_obj, microbit_instrument_set_filter2);

STATIC mp_obj_t microbit_instrument_set_envelope(mp_obj_t self_in, mp_obj_t value_in) {
    return set_instrument_object_field(self_in, offsetof(microbit_instrument_obj_t, envelope), value_in);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_instrument_set_envelope_obj, microbit_instrument_set_envelope);


STATIC const mp_map_elem_t microbit_instrument_locals_dict_table[] = {
    // TO DO -- Add some functionality
    { MP_OBJ_NEW_QSTR(MP_QSTR_copy), (mp_obj_t)&microbit_instrument_copy_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_detune1), (mp_obj_t)&microbit_instrument_set_detune1_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_detune2), (mp_obj_t)&microbit_instrument_set_detune2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_volume1), (mp_obj_t)&microbit_instrument_set_volume1_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_volume2), (mp_obj_t)&microbit_instrument_set_volume2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_source1), (mp_obj_t)&microbit_instrument_set_source1_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_source2), (mp_obj_t)&microbit_instrument_set_source2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_filter1), (mp_obj_t)&microbit_instrument_set_filter1_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_filter2), (mp_obj_t)&microbit_instrument_set_filter2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_envelope), (mp_obj_t)&microbit_instrument_set_envelope_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_instrument_locals_dict, microbit_instrument_locals_dict_table);

STATIC void microbit_instrument_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind __attribute__ ((unused))) {
    microbit_instrument_obj_t *self = (microbit_instrument_obj_t*)self_in;
    mp_printf(print, "Instrument(");
    if (self->source1 == NULL) {
        mp_printf(print, "None");
    } else {
        ((mp_obj_base_t *)self->source1)->type->print(print, self->source1, kind);
    }
    mp_printf(print, ", %f, ", fixed_to_float(self->detune1));
    if (self->filter1 == NULL) {
        mp_printf(print, "None");
    } else {
        ((mp_obj_base_t *)self->filter1)->type->print(print, self->filter1, kind);
    }
    mp_printf(print, ", ");
    if (self->source2 == NULL) {
        mp_printf(print, "None");
    } else {
        ((mp_obj_base_t *)self->source2)->type->print(print, self->source2, kind);
    }
    mp_printf(print, ", %f, ", fixed_to_float(self->detune2));
    if (self->filter2 == NULL) {
        mp_printf(print, "None");
    } else {
        ((mp_obj_base_t *)self->filter2)->type->print(print, self->filter2, kind);
    }
    mp_printf(print, ", %f, %f, ", fixed_to_float(self->balance.left_volume), fixed_to_float(self->balance.right_volume));
    ((mp_obj_base_t *)self->envelope)->type->print(print, self->envelope, kind);
    mp_printf(print, ")");
}

const mp_obj_type_t microbit_instrument_type = {
    { &mp_type_type },
    .name = MP_QSTR_Instrument,
    .print = microbit_instrument_print,
    .make_new = microbit_instrument_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    /* .locals_dict = */ (mp_obj_t)&microbit_instrument_locals_dict,
};

STATIC mp_obj_t microbit_instrument_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 9, 9, false);
    microbit_instrument_obj_t *result = m_new_obj(microbit_instrument_obj_t);
    result->base.type = &microbit_instrument_type;
    //typecheck("source1", args[0], &microbit_component_type, "Component");
    float detune1 = mp_obj_get_float(args[1]);
    //typecheck("filter1", args[2], &microbit_component_type, "Component");
    //typecheck("source2", args[3], &microbit_component_type, "Component");
    float detune2 = mp_obj_get_float(args[4]);
    //typecheck("filter2", args[5], &microbit_component_type, "Component");
    float left = mp_obj_get_float(args[6]);
    float right = mp_obj_get_float(args[7]);
    //typecheck("envelope", args[8], &microbit_envelope_type, "Envelope");
    result->source1 = args[0];
    result->detune1 = float_to_fixed(detune1, SCALE_SHIFT);
    result->filter1 = args[2];
    result->source2 = args[3];
    result->detune2 = float_to_fixed(detune2, SCALE_SHIFT);
    result->filter2 = args[5];
    result->balance.left_volume = float_to_fixed(left, SCALE_SHIFT);
    result->balance.right_volume = float_to_fixed(right, SCALE_SHIFT);
    result->envelope = args[8];
    return result;
}

static int32_t noop(int32_t signal, FilterComponent *s __attribute__ ((unused))) {
    return signal;
}

void initialise_component(mp_obj_t obj, FilterComponent *component) {
    if (obj == NULL || obj == mp_const_none) {
        component->action = noop;
        component->init = noop_init;
        return;
    }
    if (mp_obj_get_type(obj) == &microbit_envelope_type) {
        component->envelope.initialiseFrom(&((microbit_envelope_obj_t *)obj)->settings);
        component->action = adsr_quiet;
        if (((microbit_envelope_obj_t *)obj)->attack_release) {
            component->init = ar_init;
        } else {
            component->init = adsr_init;
        }
    } else if (mp_obj_get_type(obj) == &microbit_oscillator_type) {
        component->action = ((microbit_oscillator_obj_t *)obj)->waveform;
        component->oscillator.edge = ((microbit_oscillator_obj_t *)obj)->edge;
        component->oscillator.phase = 0;
        component->init = noop_init;
    } else if (mp_obj_get_type(obj) == &microbit_lowpass_type) {
        component->action = lowpass;
        component->lowpass.a = ((microbit_lowpass_obj_t *)obj)->settings.a;
        component->lowpass.b = ((microbit_lowpass_obj_t *)obj)->settings.b;
        component->init = noop_init;
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                                           "Unexpected type when initialising component"));
    }
}


mp_obj_t synth__init__() {
    synth_init();
    high_freq_pwm_init(MICROBIT_PIN_P1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(synth___init___obj, synth__init__);

mp_obj_t synth_play(mp_obj_t instrument_in, mp_obj_t music) {
    if (!high_freq_pwm_running()) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                "synth is not running."));
    }
    if (mp_obj_get_type(instrument_in) != &microbit_instrument_type) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                                           "first argument must be an instrument"));
    }
    microbit_instrument_obj_t *instrument = (microbit_instrument_obj_t *)instrument_in;

    // get either a single note or a list of notes
    mp_uint_t len;
    mp_obj_t *items;

    if (MP_OBJ_IS_STR_OR_BYTES(music)) {
        len = 1;
        items = &music;
    } else {
        mp_obj_get_array(music, &len, &items);
    }
    for (mp_uint_t i = 0; i < len; i++) {
        mp_uint_t note_len;
        const char *note_str = mp_obj_str_get_data(items[i], &note_len);
        MusicNote note = parse_note(note_str, note_len);
        int32_t duration = note.duration*50;
        if (note.period_us == 0) {
            // Rest
            mp_hal_delay_ms(duration);
        } else {
            uint32_t freq = 1000000000/note.period_us;
            if (note.octave_shift < 0) {
                freq >>= -note.octave_shift;
            } else {
                freq <<= note.octave_shift;
            }
            Voice *v = theSynth->start(instrument, freq);
            mp_hal_delay_ms(duration-10);
            v->release();
            mp_hal_delay_ms(10);
        }
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_synth_play_obj, synth_play);

extern volatile int32_t synth_interrupts;
extern volatile int32_t synth_max_delay;

extern int32_t fade_in(int32_t phase_delta, FilterComponent *s);

static int32_t setup_fade(int32_t fade, int32_t dc_level, int32_t time_ms) {
    int level = theSynth->out_level;
    if (time_ms > 0) {
        theSynth->out_level = SCALED_ONE;
        // Set left channel to dc.
        theSynth->voices[0].source1.action = dc;
        theSynth->voices[0].filter1.action = noop;
        theSynth->voices[0].balance.left_volume = SCALED_ONE*dc_level;
        // Set right channel to fade wave
        theSynth->voices[0].phase_delta2 = (1<<27)/time_ms;
        theSynth->voices[0].source2.action = fade_in;
        theSynth->voices[0].source2.oscillator.phase = 0;
        theSynth->voices[0].filter2.action = noop;
        theSynth->voices[0].balance.right_volume = SCALED_ONE*fade;
        theSynth->voices[0].envelope.action = noop;
        theSynth->voices[0].envelope.envelope.active = true;
        theSynth->delta = 0;
    }
    return level;
}

static mp_obj_t synth_stop(mp_uint_t n_args, const mp_obj_t *args) {
    int time_ms;
    if (n_args == 0) {
        time_ms = 50;
    } else {
        time_ms = mp_obj_int_get_checked(args[0]);
    }
    /* Drop from 0 to -1 */
    int level = setup_fade(-1, 0, time_ms);
    mp_hal_delay_ms(time_ms);
    high_freq_pwm_stop();
    non_blocking_timer_revert(micropython_ticker);
    theSynth->voices[0].envelope.envelope.active = false;
    theSynth->out_level = level;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_synth_stop_obj, 0, 1, synth_stop);

static mp_obj_t synth_start(mp_uint_t n_args, const mp_obj_t *args) {
    int time_ms;
    if (n_args == 0) {
        time_ms = 50;
    } else {
        time_ms = mp_obj_int_get_checked(args[0]);
    }
    theSynth->reset();
    // Need to stop main ticker as it messes with interrupts.
    // Use non-blocking ticker.
    non_blocking_timer_take_over(micropython_ticker);
    /* Ramp up from -1 to 0 */
    int level = setup_fade(1, -1, time_ms);
    //Prime the synth samples.
    for (int i = 0; i < 8; i++) {
        high_freq_pwm_signals[i&3]();
    }
    high_freq_pwm_start();
    mp_hal_delay_ms(time_ms);
    theSynth->voices[0].envelope.envelope.active = false;
    theSynth->out_level = level;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_synth_start_obj, 0, 1, synth_start);

static mp_obj_t synth_is_running(void) {
    return mp_obj_new_bool(high_freq_pwm_running());
}
static MP_DEFINE_CONST_FUN_OBJ_0(microbit_synth_is_running_obj, synth_is_running);

#define SYNTH_TEST 1

#if SYNTH_TEST
static mp_obj_t microbit_synth_get_values(mp_obj_t instrument_in, mp_obj_t release_in, mp_obj_t count_in) {
    if (mp_obj_get_type(instrument_in) != &microbit_instrument_type) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                                           "first argument must be an instrument"));
    }
    microbit_instrument_obj_t *instrument = (microbit_instrument_obj_t *)instrument_in;
    int release = mp_obj_int_get_checked(release_in);
    int count = mp_obj_int_get_checked(count_in);
    Voice *v = theSynth->start(instrument, 30000);
    for (int i = 0; i < count; i++) {
        if (i == release) {
            v->release();
        }
        int sample = high_freq_pwm_signals[i&3]();
        mp_printf(&mp_plat_print, "%d\r\n", sample);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(microbit_synth_get_values_obj, microbit_synth_get_values);
#endif

STATIC const mp_map_elem_t globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_synth) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_sine_wave), (mp_obj_t)&microbit_sine_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_saw_wave), (mp_obj_t)&microbit_saw_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_square_wave), (mp_obj_t)&microbit_square_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_triangle_wave), (mp_obj_t)&microbit_triangle_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_noise_wave), (mp_obj_t)&microbit_noise_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power4_wave), (mp_obj_t)&microbit_power4_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_step_wave), (mp_obj_t)&microbit_step_oscillator_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_fade_in), (mp_obj_t)&microbit_fade_in_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_rectangular_wave), (mp_obj_t)&microbit_make_rectangular_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), (mp_obj_t)&synth___init___obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_play), (mp_obj_t)&microbit_synth_play_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_STRINGS), (mp_obj_t)&microbit_strings_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_BELL), (mp_obj_t)&microbit_bell_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_DRUM), (mp_obj_t)&microbit_drum_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_BUZZER), (mp_obj_t)&microbit_buzzer_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_ORGAN), (mp_obj_t)&microbit_organ_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_start), (mp_obj_t)&microbit_synth_start_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_stop), (mp_obj_t)&microbit_synth_stop_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_running), (mp_obj_t)&microbit_synth_is_running_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Envelope), (mp_obj_t)&microbit_envelope_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_LowPass), (mp_obj_t)&microbit_lowpass_type},
    { MP_OBJ_NEW_QSTR(MP_QSTR_Instrument), (mp_obj_t)&microbit_instrument_type},
#if SYNTH_TEST
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_values), (mp_obj_t)&microbit_synth_get_values_obj},
#endif
};

STATIC MP_DEFINE_CONST_DICT(module_globals, globals_table);

const mp_obj_module_t synth_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_synth,
    .globals = (mp_obj_dict_t*)&module_globals,
};


}


