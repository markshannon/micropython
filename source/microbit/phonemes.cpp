


/*********************
 * Phoneme precedence (used for merging)
 *
 *
  Precedence    Type                         Phonemes
//     2       All vowels                   IY, IH, etc.
//     5       Diphthong endings            YX, WX, ER
//     8       Terminal liquid consonants   LX, WX, YX, N, NX
//     9       Liquid consonants            L, RX, W
//     10      Glide                        R, OH
//     11      Glide                        WH
//     18      Voiceless fricatives         S, SH, F, TH
//     20      Voiced fricatives            Z, ZH, V, DH
//     23      Plosives, stop consonants    P, T, K, KX, DX, CH
//     26      Stop consonants              J, GX, B, D, G
//     27-29   Stop consonants (internal)   **
//     30      Unvoiced consonants          /H, /X and Q*
//     160     Nasal                        M

 *
 *
 */


extern "C" {

#include "microbit/modmicrobit.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "microbit/phonemes.h"

/*
const mp_obj_type_t microbit_phoneme_type = {
    { &mp_type_type },
    .name = MP_QSTR_Phoneme,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = phoneme_iter,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};


microbit_sound_bytes_obj_t microbit_phoneme_M = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_M,
    .sample = &microbit_sample_M_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_Ay = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_Ay,
    .sample = &microbit_sample_Ay_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_K = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_K,
    .sample = &microbit_sample_K_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_R = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_R,
    .sample = &microbit_sample_R_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_Oh = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_Oh,
    .sample = &microbit_sample_Oh_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_B = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_B,
    .sample = &microbit_sample_B_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_Ih = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_Ih,
    .sample = &microbit_sample_Ih_sample_obj,
    .cls = VOICED_CONSONANT
};

microbit_sound_bytes_obj_t microbit_phoneme_T = {
    .base = { &microbit_phoneme_type },
    .length = 9,
    .blend_in = 2,
    .blend_out = 2,
    .blend_rank = 2,
    .name = MP_QSTR_T,
    .sample = &microbit_sample_T_sample_obj,
    .cls = VOICED_CONSONANT
};

*/

}