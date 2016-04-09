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
#ifndef __MICROPY_INCLUDED_PERSISTENCE_H__
#define __MICROPY_INCLUDED_PERSISTENCE_H__

#include "nrf51.h"
#include "nrf_nvmc.h"
#include "py/lexer.h"

inline uint32_t persistent_page_size(void) {
    return NRF_FICR->CODEPAGESIZE;
}

bool is_persistent_page_aligned(const void *ptr);
void persistent_write(const void *dest, const void *src, uint32_t byte_count);
void persistent_write_unchecked(const void *dest, const void *src, uint32_t byte_count);
void persistent_write_byte(const uint8_t *dest, const uint8_t val);
void persistent_erase(const void *dest, uint32_t byte_count);
void persistent_write_byte_unchecked(const uint8_t *dest, const uint8_t val);

typedef struct _file_descriptor_obj {
    mp_obj_base_t base;
    uint8_t start_chunk;
    uint8_t seek_chunk;
    uint8_t seek_offset;
    uint8_t end_chunk;
    uint8_t end_offset;
    bool writable;
    bool open;
    bool binary;
} file_descriptor_obj;

#define LOG_CHUNK_SIZE 7
#define CHUNK_SIZE (1<<LOG_CHUNK_SIZE)
#define DATA_PER_CHUNK (CHUNK_SIZE-2)

#define UNUSED_CHUNK 255
#define FREED_CHUNK  0
#define FILE_START 254

#define MAX_FILENAME_LENGTH 40


typedef struct _file_header {
    uint8_t end_chunk;
    uint8_t end_offset;
    uint8_t name_len;
    char filename[MAX_FILENAME_LENGTH];
} file_header;

typedef struct _file_chunk {
    uint8_t marker;
    union {
        char data[DATA_PER_CHUNK];
        file_header header;
    };
    uint8_t next_chunk;
} file_chunk;

#define FILE_NOT_FOUND ((uint8_t)-1)

#define CHUNKS_IN_FILE_SYSTEM 160

#define STATIC_ASSERT(e) extern char static_assert_failed[(e) ? 1 : -1]

uint8_t microbit_find_file(const char *name, int name_len);
file_descriptor_obj *microbit_open(const char *name, uint32_t name_len, bool write, bool binary);
mp_lexer_t *microbit_file_lexer(qstr src_name, file_descriptor_obj *fd);

#define min(a,b) (((a)<(b))?(a):(b))

#endif // __MICROPY_INCLUDED_PERSISTENCE_H__
