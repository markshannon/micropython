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

#include "py/obj.h"
#include "filesystem.h"

STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_remove_obj, microbit_remove);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(microbit_file_list_obj, microbit_file_list);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_file_size_obj, microbit_file_size);

static const mp_map_elem_t _globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_os) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_remove), (mp_obj_t)&microbit_remove_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_listdir), (mp_obj_t)&microbit_file_list_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_size), (mp_obj_t)&microbit_file_size_obj },
};

static MP_DEFINE_CONST_DICT(_globals, _globals_table);

const mp_obj_module_t os_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_os,
    .globals = (mp_obj_dict_t*)&_globals,
};
