
#ifndef __MICROPY_INCLUDED_MICROBIT_PHONEMES_H__
#define __MICROPY_INCLUDED_MICROBIT_PHONEMES_H__

#include "modsound.h"

enum PhonemeClass {
    SHORT_VOWEL,
    LONG_VOWEL,
    VOICED_CONSONANT,
    VOICELESS_CONSONANT,
    DIPTHONG,
    H
};

typedef struct _microbit_phoneme_obj_t {
    mp_obj_base_t base;
    uint8_t length;
    uint8_t blend_in;
    uint8_t blend_out;
    uint8_t blend_rank;
    mp_obj_t name;
    microbit_sample_t *sample;
    PhonemeClass cls;
} microbit_phoneme_obj_t;


#endif //  __MICROPY_INCLUDED_MICROBIT_PHONEMES_H__