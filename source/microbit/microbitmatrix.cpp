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

#include "microbitmatrix.h"

#include "py/runtime0.h"
#include "py/runtime.h"
#include "modmicrobit.h"

static inline float get(microbit_matrix_obj_t *self, mp_uint_t row, mp_uint_t col) {
    return self->data[row*self->columns+col];
}

static inline void set(microbit_matrix_obj_t *self, mp_uint_t row, mp_uint_t col, float val) {
    self->data[row*self->columns+col] = val;
}

float microbit_matrix_get(microbit_matrix_obj_t *m, mp_uint_t row, mp_uint_t col) {
    return get(m, row, col);
}

void microbit_matrix_set(microbit_matrix_obj_t *m, mp_uint_t row, mp_uint_t col, float val) {
    set(m, row, col, val);
}

static mp_obj_t microbit_matrix_new(const mp_obj_type_t *type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    mp_obj_t data = args[0];
    mp_int_t rows = mp_obj_get_int(mp_obj_len(data));
    mp_obj_t first_row = mp_obj_subscr(data, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_SENTINEL);
    mp_int_t columns = mp_obj_get_int(mp_obj_len(first_row));
    for (mp_int_t y = 1; y < rows; y++) {
        mp_obj_t row = mp_obj_subscr(data, MP_OBJ_NEW_SMALL_INT(y), MP_OBJ_SENTINEL);
        if (mp_obj_get_int(mp_obj_len(row)) != columns) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Differing row lengths"));
        }
    }
    microbit_matrix_obj_t *result = microbit_make_matrix(rows, columns);
    for (mp_int_t y = 0; y < rows; y++) {
        mp_obj_t row = mp_obj_subscr(data, MP_OBJ_NEW_SMALL_INT(y), MP_OBJ_SENTINEL);
        for (mp_int_t x = 0; x < columns; x++) {
            float val = mp_obj_get_float(mp_obj_subscr(row, MP_OBJ_NEW_SMALL_INT(x), MP_OBJ_SENTINEL));
            set(result, y, x, val);
        }
    }
    return result;
}

static mp_obj_t matrix_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value_in) {
    microbit_matrix_obj_t *self = (microbit_matrix_obj_t *)self_in;
    if (mp_obj_get_type(index_in) != &mp_type_tuple) {
        return MP_OBJ_NULL; // op not supported
    }
    mp_uint_t len;
    mp_obj_t *items;
    mp_obj_tuple_get(index_in, &len, &items);
    if (len != 2) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Wrong dimensionality"));
    }
    mp_uint_t row = mp_obj_get_int(items[0]);
    mp_uint_t col = mp_obj_get_int(items[1]);
    if (row >= self->rows || col >= self->columns) {
         nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "index out of bounds"));
    }
    if (value_in == MP_OBJ_SENTINEL) {
        // load
        return mp_obj_new_float(get(self, row, col));
    } else {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "Cannot modify matrix"));
    }
}

microbit_matrix_obj_t *microbit_matrix_transpose(microbit_matrix_obj_t *self) {
    microbit_matrix_obj_t *result = microbit_make_matrix(self->columns, self->rows);
    for (uint32_t x = 0; x < self->rows; x++) {
        for (uint32_t y = 0; y < self->columns; y++) {
            set(result, y, x, get(self, x, y));
        }
    }
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_matrix_transpose_obj, microbit_matrix_transpose);


static mp_obj_t matrix_width(mp_obj_t self_in) {
    microbit_matrix_obj_t *self = (microbit_matrix_obj_t *)self_in;
    return MP_OBJ_NEW_SMALL_INT(self->columns);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_matrix_width_obj, matrix_width);

static mp_obj_t matrix_height(mp_obj_t self_in) {
    microbit_matrix_obj_t *self = (microbit_matrix_obj_t *)self_in;
    return MP_OBJ_NEW_SMALL_INT(self->rows);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_matrix_height_obj, matrix_height);

microbit_matrix_obj_t *microbit_matrix_invert_maybe(microbit_matrix_obj_t *self) {
    // We only support square matrices of size 4...
    if (self->rows != 4 || self->columns != 4) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Not a 4x4 matrix"));
    }
    microbit_matrix_obj_t *result = microbit_make_matrix(4, 4);

#define M(x, y) get(self, x, y)

    float s0 = M(0, 0) * M(1, 1) - M(1, 0) * M(0, 1);
    float s1 = M(0, 0) * M(1, 2) - M(1, 0) * M(0, 2);
    float s2 = M(0, 0) * M(1, 3) - M(1, 0) * M(0, 3);
    float s3 = M(0, 1) * M(1, 2) - M(1, 1) * M(0, 2);
    float s4 = M(0, 1) * M(1, 3) - M(1, 1) * M(0, 3);
    float s5 = M(0, 2) * M(1, 3) - M(1, 2) * M(0, 3);

    float c5 = M(2, 2) * M(3, 3) - M(3, 2) * M(2, 3);
    float c4 = M(2, 1) * M(3, 3) - M(3, 1) * M(2, 3);
    float c3 = M(2, 1) * M(3, 2) - M(3, 1) * M(2, 2);
    float c2 = M(2, 0) * M(3, 3) - M(3, 0) * M(2, 3);
    float c1 = M(2, 0) * M(3, 2) - M(3, 0) * M(2, 2);
    float c0 = M(2, 0) * M(3, 1) - M(3, 0) * M(2, 1);

    set(result, 0, 0, M(1, 1) * c5 - M(1, 2) * c4 + M(1, 3) * c3);
    set(result, 0, 1, -M(0, 1) * c5 + M(0, 2) * c4 - M(0, 3) * c3);
    set(result, 0, 2, M(3, 1) * s5 - M(3, 2) * s4 + M(3, 3) * s3);
    set(result, 0, 3, -M(2, 1) * s5 + M(2, 2) * s4 - M(2, 3) * s3);

    set(result, 1, 0, -M(1, 0) * c5 + M(1, 2) * c2 - M(1, 3) * c1);
    set(result, 1, 1, M(0, 0) * c5 - M(0, 2) * c2 + M(0, 3) * c1);
    set(result, 1, 2, -M(3, 0) * s5 + M(3, 2) * s2 - M(3, 3) * s1);
    set(result, 1, 3, M(2, 0) * s5 - M(2, 2) * s2 + M(2, 3) * s1);

    set(result, 2, 0, M(1, 0) * c4 - M(1, 1) * c2 + M(1, 3) * c0);
    set(result, 2, 1, -M(0, 0) * c4 + M(0, 1) * c2 - M(0, 3) * c0);
    set(result, 2, 2, M(3, 0) * s4 - M(3, 1) * s2 + M(3, 3) * s0);
    set(result, 2, 3, -M(2, 0) * s4 + M(2, 1) * s2 - M(2, 3) * s0);

    set(result, 3, 0, -M(1, 0) * c3 + M(1, 1) * c1 - M(1, 2) * c0);
    set(result, 3, 1, M(0, 0) * c3 - M(0, 1) * c1 + M(0, 2) * c0);
    set(result, 3, 2, -M(3, 0) * s3 + M(3, 1) * s1 - M(3, 2) * s0);
    set(result, 3, 3, M(2, 0) * s3 - M(2, 1) * s1 + M(2, 2) * s0);

    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    if (det == 0) {
        return NULL;
    }
    float invdet = 1.0f / det;
    for (int i = 0; i < 16; i++) {
        result->data[i] *= invdet;
    }

    return result;

#undef M

}


microbit_matrix_obj_t *microbit_matrix_invert(microbit_matrix_obj_t *self) {
    microbit_matrix_obj_t *result = microbit_matrix_invert_maybe(self);
    if (result == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Singular matrix"));
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_matrix_invert_obj, microbit_matrix_invert);



STATIC const mp_map_elem_t microbit_matrix_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_invert), (mp_obj_t)&microbit_matrix_invert_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_transpose), (mp_obj_t)&microbit_matrix_transpose_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_width), (mp_obj_t)&microbit_matrix_width_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_height), (mp_obj_t)&microbit_matrix_height_obj },
};
STATIC MP_DEFINE_CONST_DICT(microbit_matrix_locals_dict, microbit_matrix_locals_dict_table);

static mp_obj_t matrix_equals(microbit_matrix_obj_t *self, microbit_matrix_obj_t *other) {
    if (self->rows != other->rows || self->columns != other->columns) {
        return mp_const_false;
    }
    for (uint32_t col = 0; col < self->columns; col++) {
        for (uint32_t row = 0; row < self->rows; row++) {
            if (get(self, row, col) != get(other, row, col)) {
                return mp_const_false;
            }
        }
    }
    return mp_const_true;
}

microbit_matrix_obj_t *microbit_matrix_add(microbit_matrix_obj_t *self, microbit_matrix_obj_t *other) {
    if (self->rows != other->rows || self->columns != other->columns) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Mismatched matrices"));
    }
    microbit_matrix_obj_t *result = microbit_make_matrix(self->rows, other->columns);
    for (uint32_t x = 0; x < result->columns; x++) {
        for (uint32_t y = 0; y < result->rows; y++) {
            set(result, x, y, get(self, x, y)+get(other, x, y));
        }
    }
    return result;

}

microbit_matrix_obj_t *microbit_matrix_mult(microbit_matrix_obj_t *self, microbit_matrix_obj_t *other) {
    if (self->columns != other->rows) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Mismatched matrices"));
    }
    microbit_matrix_obj_t *result = microbit_make_matrix(self->rows, other->columns);
    for (uint32_t x = 0; x < result->columns; x++) {
        for (uint32_t y = 0; y < result->rows; y++) {
            float v = 0.0f;
            for (int i = 0; i < self->columns; i++) {
                v += get(self, y, i) * get(other, i, x);
            }
            set(result, y, x, v);
        }
    }
    return result;
}

STATIC mp_obj_t matrix_binary_op(mp_uint_t op, mp_obj_t self_in, mp_obj_t other_in) {
    if (mp_obj_get_type(other_in) != &microbit_matrix_type) {
        return MP_OBJ_NULL; // op not supported
    }
    microbit_matrix_obj_t *self = (microbit_matrix_obj_t *)self_in;
    microbit_matrix_obj_t *other = (microbit_matrix_obj_t *)other_in;
    if (op == MP_BINARY_OP_EQUAL) {
        return matrix_equals(self, other);
    }
    if (op == MP_BINARY_OP_MULTIPLY) {
        return microbit_matrix_mult(self, other);
    }
    if (op == MP_BINARY_OP_ADD) {
        return microbit_matrix_add(self, other);
    }
    return MP_OBJ_NULL; // op not supported
}

static void microbit_matrixprint(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    microbit_matrix_obj_t *self = (microbit_matrix_obj_t *)self_in;
    mp_printf(print, "Matrix([ ");
    for (int y = 0; y < self->rows; ++y) {
        if (kind == PRINT_STR)
            mp_printf(print, "\n  ");
        mp_printf(print, "[ ");
        for (int x = 0; x < self->columns; ++x) {
            mp_printf(print, "%f, ", get(self, x, y));
        }
        mp_printf(print, "], ");
    }
    if (kind == PRINT_STR)
        mp_printf(print, "\n");
    mp_printf(print, "])");
}

const mp_obj_type_t microbit_matrix_type = {
    { &mp_type_type },
    .name = MP_QSTR_Matrix,
    .print = microbit_matrixprint,
    .make_new = microbit_matrix_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = matrix_binary_op,
    .attr = NULL,
    .subscr = matrix_subscr,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = { NULL },
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_matrix_locals_dict,
};

microbit_matrix_obj_t *microbit_make_matrix( mp_int_t rows, mp_int_t columns) {
    microbit_matrix_obj_t *result = m_new_obj_var(microbit_matrix_obj_t, float, (rows*columns-1));
    result->base.type = &microbit_matrix_type;
    result->rows = rows;
    result->columns = columns;
    return result;
}

}
