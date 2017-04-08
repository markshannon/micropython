/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
 * Copyright (c) 2016 Damien P. George
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

#define rand30() (microbit_pseudo_random(0x40000000))
#define randbelow(n) (microbit_pseudo_random(n))

extern "C" {

#include <assert.h>
#include <string.h>
#include <string.h>

#include "py/runtime.h"
#include "device.h"

static uint32_t random_value;

static void microbit_seed_random(void) {
    random_value = 0;

    /* Just power up hardware RNG for minimum time necessary */
    NRF_RNG->TASKS_START = 1;
    for(int i = 0; i < 4; i++)
    {
        // Wait for ready event
        NRF_RNG->EVENTS_VALRDY = 0;
        while(NRF_RNG->EVENTS_VALRDY == 0);

        random_value |= NRF_RNG->VALUE << (i*8);
    }
    NRF_RNG->TASKS_STOP = 1;
}

/* Reuse the PRNG from the DAL.
 * It is not the best PRNG available in 2017,
 * but it is adequate.
 */
static int microbit_pseudo_random(int max)
{
    uint32_t m, result;

    // Our maximum return value is actually one less than passed
    max--;

    do {
        m = (uint32_t)max;
        result = 0;
        do {
            // Cycle the LFSR (Linear Feedback Shift Register).
            // We use an optimal sequence with a period of 2^32-1, as defined by Bruce Schneier here (a true legend in the field!),
            // For those interested, it's documented in his paper:
            // "Pseudo-Random Sequence Generator for 32-Bit CPUs: A fast, machine-independent generator for 32-bit Microprocessors"
            // https://www.schneier.com/paper-pseudorandom-sequence.html
            uint32_t rnd = random_value;

            rnd = ((((rnd >> 31)
                          ^ (rnd >> 6)
                          ^ (rnd >> 4)
                          ^ (rnd >> 2)
                          ^ (rnd >> 1)
                          ^ rnd)
                          & 0x0000001)
                          << 31 )
                          | (rnd >> 1);

            random_value = rnd;

            result = ((result << 1) | (rnd & 0x00000001));
        } while(m >>= 1);
    } while (result > (uint32_t)max);


    return result;
}


STATIC mp_obj_t mod_random_getrandbits(mp_obj_t num_in) {
    int n = mp_obj_get_int(num_in);
    if (n > 30 || n == 0) {
        nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
    }
    uint32_t mask = ~0;
    // Beware of C undefined behavior when shifting by >= than bit size
    mask >>= (32 - n);
    return mp_obj_new_int_from_uint(rand30() & mask);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_random_getrandbits_obj, mod_random_getrandbits);

STATIC mp_obj_t mod_random_seed(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0 || args[0] == mp_const_none) {
        microbit_seed_random();
    } else {
        mp_uint_t seed = mp_obj_get_int_truncated(args[0]);
        random_value = seed;
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_random_seed_obj, 0, 1, mod_random_seed);

STATIC mp_obj_t mod_random_randrange(size_t n_args, const mp_obj_t *args) {
    mp_int_t start = mp_obj_get_int(args[0]);
    if (n_args == 1) {
        // range(stop)
        if (start > 0) {
            return mp_obj_new_int(randbelow(start));
        } else {
            nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
        }
    } else {
        mp_int_t stop = mp_obj_get_int(args[1]);
        if (n_args == 2) {
            // range(start, stop)
            if (start < stop) {
                return mp_obj_new_int(start + randbelow(stop - start));
            } else {
                nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
            }
        } else {
            // range(start, stop, step)
            mp_int_t step = mp_obj_get_int(args[2]);
            mp_int_t n;
            if (step > 0) {
                n = (stop - start + step - 1) / step;
            } else if (step < 0) {
                n = (stop - start + step + 1) / step;
            } else {
                nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
            }
            if (n > 0) {
                return mp_obj_new_int(start + step * randbelow(n));
            } else {
                nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
            }
        }
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_random_randrange_obj, 1, 3, mod_random_randrange);

STATIC mp_obj_t mod_random_randint(mp_obj_t a_in, mp_obj_t b_in) {
    mp_int_t a = mp_obj_get_int(a_in);
    mp_int_t b = mp_obj_get_int(b_in);
    if (a <= b) {
        return mp_obj_new_int(a + randbelow(b - a + 1));
    } else {
        nlr_raise(mp_obj_new_exception(&mp_type_ValueError));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_random_randint_obj, mod_random_randint);

STATIC mp_obj_t mod_random_choice(mp_obj_t seq) {
    mp_int_t len = mp_obj_get_int(mp_obj_len(seq));
    if (len > 0) {
        return mp_obj_subscr(seq, mp_obj_new_int(randbelow(len)), MP_OBJ_SENTINEL);
    } else {
        nlr_raise(mp_obj_new_exception(&mp_type_IndexError));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_random_choice_obj, mod_random_choice);

#if MICROPY_PY_BUILTINS_FLOAT

// returns a number in the range [0..1) using RNG to fill in the fraction bits
STATIC mp_float_t randfloat(void) {
    #if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
    typedef uint64_t mp_float_int_t;
    #elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
    typedef uint32_t mp_float_int_t;
    #endif
    union {
        mp_float_t f;
        #if MP_ENDIANNESS_LITTLE
        struct { mp_float_int_t frc:MP_FLOAT_FRAC_BITS, exp:MP_FLOAT_EXP_BITS, sgn:1; } p;
        #else
        struct { mp_float_int_t sgn:1, exp:MP_FLOAT_EXP_BITS, frc:MP_FLOAT_FRAC_BITS; } p;
        #endif
    } u;
    u.p.sgn = 0;
    u.p.exp = (1 << (MP_FLOAT_EXP_BITS - 1)) - 1;
    if (MP_FLOAT_FRAC_BITS <= 30) {
        u.p.frc = rand30();
    } else {
        u.p.frc = ((uint64_t)rand30() << 30) | (uint64_t)rand30();
    }
    return u.f - 1;
}

STATIC mp_obj_t mod_random_random(void) {
    return mp_obj_new_float(randfloat());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_random_random_obj, mod_random_random);

STATIC mp_obj_t mod_random_uniform(mp_obj_t a_in, mp_obj_t b_in) {
    mp_float_t a = mp_obj_get_float(a_in);
    mp_float_t b = mp_obj_get_float(b_in);
    return mp_obj_new_float(a + (b - a) * randfloat());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_random_uniform_obj, mod_random_uniform);

#endif

STATIC const mp_rom_map_elem_t mp_module_random_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_random) },
    { MP_ROM_QSTR(MP_QSTR_getrandbits), MP_ROM_PTR(&mod_random_getrandbits_obj) },
    { MP_ROM_QSTR(MP_QSTR_seed), MP_ROM_PTR(&mod_random_seed_obj) },
    { MP_ROM_QSTR(MP_QSTR_randrange), MP_ROM_PTR(&mod_random_randrange_obj) },
    { MP_ROM_QSTR(MP_QSTR_randint), MP_ROM_PTR(&mod_random_randint_obj) },
    { MP_ROM_QSTR(MP_QSTR_choice), MP_ROM_PTR(&mod_random_choice_obj) },
    { MP_ROM_QSTR(MP_QSTR_random), MP_ROM_PTR(&mod_random_random_obj) },
    { MP_ROM_QSTR(MP_QSTR_uniform), MP_ROM_PTR(&mod_random_uniform_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_random_globals, mp_module_random_globals_table);

const mp_obj_module_t random_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_random,
    .globals = (mp_obj_dict_t*)&mp_module_random_globals,
};

}
