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
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/gc.h"
#include "py/stream.h"
#include "modfile.h"
#include "lib/ticker.h"

#define DEBUG_FILE 0
#if DEBUG_FILE
#define DEBUG(s) printf s
#else
#define DEBUG(s) (void)0
#endif


/**  How it works:
 * The File System consists of CHUNKS_IN_FILE_SYSTEM chunks of CHUNK_SIZE each.
 * Each chunk consists of a one byte marker and a one byte tail
 * The marker shows whether this chunk is the start of a file, the midst of a file
 * (in which case it refers to the previous chunk in the file) or whether it is UNUSED
 * (and erased) or FREED (which means it is unused, but not erased).
 * Chunks are selected in a randomised round-robin fashion to even out wear on the flash
 * memory as much as possible.
 * A file consists of a linked list of chunks. The first chunk in a file contains its name
 * as well as the end chunk and offset.
 * Files are found by linear search of the chunks, this means that no meta-data needs to be stored
 * outside of the file, which prevents wear hot-spots. Since there are fewer than 200 chunks,
 * the search is fast enough.
 *
 * Writing to files relies on the persistent API which is high-level wrapper on top of the Nordic SDK.
 */


extern int8_t file_system_data[];

// Pages are numbered from 1 to PAGES_IN_FILE_SYSTEM, as we need to reserve 0 as the FREED marker.
static const file_chunk *file_system_chunks = &(((file_chunk *)&file_system_data)[-1]);

STATIC_ASSERT((sizeof(file_chunk) == CHUNK_SIZE));

static inline char *seek_address(file_descriptor_obj *self) {
    return (char *)&(file_system_chunks[self->seek_chunk].data[self->seek_offset]);
}

uint8_t microbit_find_file(const char *name, int name_len) {
    for (uint8_t index = 1; index <= CHUNKS_IN_FILE_SYSTEM; index++) {
        const file_chunk *p = &file_system_chunks[index];
        if (p->marker != FILE_START)
            continue;
        if (p->header.name_len != name_len)
            continue;
        if (memcmp(name, &p->header.filename[0], name_len) == 0) {
            DEBUG(("FILE DEBUG: File found. index %d\r\n", index));
            return index;
        }
    }
    DEBUG(("FILE DEBUG: File not found.\r\n"));
    return FILE_NOT_FOUND;
}

uint8_t start_index = CHUNKS_IN_FILE_SYSTEM+1;

static void randomise_start_index(void) {
    if (start_index > CHUNKS_IN_FILE_SYSTEM) {
        uint8_t new_index; // 0 based index.
        NRF_RNG->TASKS_START = 1;
        // Wait for valid number
        do {
            NRF_RNG->EVENTS_VALRDY = 0;
            while(NRF_RNG->EVENTS_VALRDY == 0);
            new_index = NRF_RNG->VALUE&255;
        } while (new_index >= CHUNKS_IN_FILE_SYSTEM);
        start_index = new_index + 1;  // Adjust index to 1 based.
        NRF_RNG->TASKS_STOP = 1;
    }
}

/** Return best available chunk, that is:
 * 1  Any UNUSED chunk, or failing that
 * 2. First aligned block in a page of FREED chunks, or failing that
 * 3. Any FREED chunk.
 * Make sure that the resulting chunk is erased.
 */
static uint8_t find_chunk_and_erase(void) {
    uint8_t candidate = FILE_NOT_FOUND;
    bool full_page_candidate = false;
    // Start search at a random chunk to spread the wear more evenly.
    randomise_start_index();
    uint8_t index = start_index;
    do {
        const file_chunk *p = &file_system_chunks[index];
        if (p->marker == UNUSED_CHUNK) {
            DEBUG(("FILE DEBUG: Unused chunk found: %d\r\n", index));
            return index;
        }
        if (!full_page_candidate && is_persistent_page_aligned(p)) {
            uint32_t i, chunks_per_page = persistent_page_size()>>LOG_CHUNK_SIZE;
            for (i = 0; i < chunks_per_page; i++) {
                if (p[i].marker != FREED_CHUNK)
                    break;
            }
            if (i == chunks_per_page) {
                candidate = index;
                full_page_candidate = true;
            }
        }
        if (p->marker == FREED_CHUNK && candidate == FILE_NOT_FOUND) {
            candidate = index;
        }
        index++;
        if (index == CHUNKS_IN_FILE_SYSTEM+1) index = 1;
    } while (index != start_index);
    if (full_page_candidate) {
        persistent_erase(&file_system_chunks[candidate], persistent_page_size());
    } else {
        persistent_erase(&file_system_chunks[candidate], sizeof(file_chunk));
    }
    DEBUG(("FILE DEBUG: Used chunk found: %d\r\n", candidate));
    return candidate;
}

static file_descriptor_obj *microbit_file_descriptor_new(uint8_t start_chunk, bool write, bool binary);


file_descriptor_obj *microbit_open(const char *name, uint32_t name_len, bool write, bool binary) {
    if (name_len > MAX_FILENAME_LENGTH) {
        return NULL;
    }
    uint8_t index = microbit_find_file(name, name_len);
    if (write) {
        if (index != FILE_NOT_FOUND) {
            // Free old file
            persistent_write_byte_unchecked(&(file_system_chunks[index].marker), FREED_CHUNK);
        }
        index = find_chunk_and_erase();
        if (index == FILE_NOT_FOUND) {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "No more storage space"));
        }
        persistent_write_byte_unchecked(&(file_system_chunks[index].marker), FILE_START);
        persistent_write_byte_unchecked(&(file_system_chunks[index].header.name_len), name_len);
        persistent_write_unchecked(&(file_system_chunks[index].header.filename[0]), name, name_len);
    } else {
        if (index == FILE_NOT_FOUND) {
            return NULL;
        }
    }
    return microbit_file_descriptor_new(index, write, binary);
}

static mp_obj_t microbit_open_func(size_t n_args, const mp_obj_t *args) {
    /// -1 means default; 0 explicitly false; 1 explicitly true.
    int read = -1;
    int text = -1;
    if (n_args == 2) {
        mp_uint_t len;
        const char *mode = mp_obj_str_get_data(args[1], &len);
        for (mp_uint_t i = 0; i < len; i++) {
            if (mode[i] == 'r' || mode[i] == 'w') {
                if (read >= 0) {
                    goto mode_error;
                }
                read = (mode[i] == 'r');
            } else if (mode[i] == 'b' || mode[i] == 't') {
                if (text >= 0) {
                    goto mode_error;
                }
                text = (mode[i] == 't');
            } else {
                goto mode_error;
            }
        }
    }
    mp_uint_t name_len;
    const char *filename = mp_obj_str_get_data(args[0], &name_len);
    file_descriptor_obj *res = microbit_open(filename, name_len, read == 0, text == 0);
    if (res == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "file not found"));
    }
    DEBUG(("FILE DEBUG: Opening file: Start chunk: %d End chunk: %d End offset: %d. Write: %d.\r\n", res->start_chunk, res->end_chunk, res->end_offset, res->writable
));
    return res;
mode_error:
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "illegal mode"));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_open_obj, 1, 2, microbit_open_func);

static void clear_file(uint8_t chunk) {
    do {
        persistent_write_byte(&(file_system_chunks[chunk].marker), FREED_CHUNK);
        DEBUG(("FILE DEBUG: Clearing chunk %d.\n", chunk));
        chunk = file_system_chunks[chunk].next_chunk;
    } while (chunk <= CHUNKS_IN_FILE_SYSTEM);
}

static mp_obj_t microbit_remove(mp_obj_t filename) {
    mp_uint_t name_len;
    const char *name = mp_obj_str_get_data(filename, &name_len);
    mp_uint_t index = microbit_find_file(name, name_len);
    if (index == 255) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "file not found"));
    }
    clear_file(index);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_remove_obj, microbit_remove);

static int advance(file_descriptor_obj *self, uint32_t n, bool write) {
    DEBUG(("FILE DEBUG: Advancing from chunk %d, offset %d.\r\n", self->seek_chunk, self->seek_offset));
    self->seek_offset += n;
    if (self->seek_offset == DATA_PER_CHUNK) {
        self->seek_offset = 0;
        if (write) {
            uint8_t next_chunk = find_chunk_and_erase();
            if (next_chunk == FILE_NOT_FOUND) {
                clear_file(self->start_chunk);
                self->open = false;
                return ENOSPC;
            }
            /* Link next chunk to this one */
            persistent_write_byte(&(file_system_chunks[self->seek_chunk].next_chunk), next_chunk);
            persistent_write_byte(&(file_system_chunks[next_chunk].marker), self->seek_chunk);
        }
        self->seek_chunk = file_system_chunks[self->seek_chunk].next_chunk;
    }
    DEBUG(("FILE DEBUG: Advanced to chunk %d, offset %d.\r\n", self->seek_chunk, self->seek_offset));
    return 0;
}

static mp_uint_t file_read(mp_obj_t obj, void *buf, mp_uint_t len, int *errcode) {
    file_descriptor_obj *self = (file_descriptor_obj *)obj;
    if (!self->open || self->writable) {
        *errcode = EBADF;
        return MP_STREAM_ERROR;
    }
    mp_uint_t to_read = DATA_PER_CHUNK - self->seek_offset;
    if (self->seek_chunk == self->end_chunk) {
        to_read = min(to_read, (mp_uint_t)self->end_offset-self->seek_offset);
    }
    to_read = min(to_read, len);
    memcpy(buf, seek_address(self), to_read);
    advance(self, to_read, false);
    return to_read;
}

static mp_uint_t file_write(mp_obj_t obj, const void *buf, mp_uint_t size, int *errcode) {
    file_descriptor_obj *self = (file_descriptor_obj *)obj;
    if (!self->open || !self->writable) {
        *errcode = EBADF;
        return MP_STREAM_ERROR;
    }
    uint32_t len = size;
    const uint8_t *data = buf;
    while (len) {
        uint32_t to_write = min(((uint32_t)(DATA_PER_CHUNK - self->seek_offset)), len);
        persistent_write(seek_address(self), data, to_write);
        int err = advance(self, to_write, true);
        if (err) {
            *errcode = err;
            return MP_STREAM_ERROR;
        }
        data += to_write;
        len -= to_write;
    }
    return size;
}

static mp_obj_t microbit_file_writable(mp_obj_t self) {
    return mp_obj_new_bool(((file_descriptor_obj *)self)->writable);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_file_writable_obj, microbit_file_writable);

static void microbit_file_close(file_descriptor_obj *fd) {
    if (fd->writable) {
        fd->end_offset = fd->seek_offset;
        fd->end_chunk = fd->seek_chunk;
        persistent_write_byte(&(file_system_chunks[fd->start_chunk].header.end_chunk), fd->end_chunk);
        persistent_write_byte(&(file_system_chunks[fd->start_chunk].header.end_offset), fd->end_offset);
    } else {
        fd->seek_offset = fd->end_offset;
        fd->seek_chunk = fd->end_chunk;
    }
    fd->open = false;
}

static mp_obj_t microbit_file_close_func(mp_obj_t self_in) {
    microbit_file_close((file_descriptor_obj *)self_in);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_file_close_obj, microbit_file_close_func);

STATIC mp_obj_t file___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return microbit_file_close_func(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(file___exit___obj, 4, 4, file___exit__);

static mp_obj_t microbit_file_name(mp_obj_t self_in) {
    file_descriptor_obj *self = (file_descriptor_obj *)self_in;
    return mp_obj_new_str(&(file_system_chunks[self->start_chunk].header.filename[0]), file_system_chunks[self->start_chunk].header.name_len, false);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_file_name_obj, microbit_file_name);

static const mp_map_elem_t microbit_bytesio_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_close), (mp_obj_t)&microbit_file_close_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_name), (mp_obj_t)&microbit_file_name_obj },
    { MP_ROM_QSTR(MP_QSTR___enter__), (mp_obj_t)&mp_identity_obj },
    { MP_ROM_QSTR(MP_QSTR___exit__), (mp_obj_t)&file___exit___obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_writable), (mp_obj_t)&microbit_file_writable_obj },
    /* Stream methods */
    { MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&mp_stream_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readall), (mp_obj_t)&mp_stream_readall_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readinto), (mp_obj_t)&mp_stream_readinto_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&mp_stream_write_obj },
};
static MP_DEFINE_CONST_DICT(microbit_bytesio_locals_dict, microbit_bytesio_locals_dict_table);

STATIC const mp_stream_p_t bytesio_stream_p = {
    .read = file_read,
    .write = file_write,
};

const mp_obj_type_t microbit_bytesio_type = {
    { &mp_type_type },
    .name = MP_QSTR_BytesIO,
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
    .stream_p = &bytesio_stream_p,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_bytesio_locals_dict,
};

static const mp_map_elem_t microbit_textio_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_close), (mp_obj_t)&microbit_file_close_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_name), (mp_obj_t)&microbit_file_name_obj },
    { MP_ROM_QSTR(MP_QSTR___enter__), (mp_obj_t)&mp_identity_obj },
    { MP_ROM_QSTR(MP_QSTR___exit__), (mp_obj_t)&file___exit___obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_writable), (mp_obj_t)&microbit_file_writable_obj },
    /* Stream methods */
    { MP_OBJ_NEW_QSTR(MP_QSTR_read), (mp_obj_t)&mp_stream_read_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readall), (mp_obj_t)&mp_stream_readall_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_readline), (mp_obj_t)&mp_stream_unbuffered_readline_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_write), (mp_obj_t)&mp_stream_write_obj },
};
static MP_DEFINE_CONST_DICT(microbit_textio_locals_dict, microbit_textio_locals_dict_table);


STATIC const mp_stream_p_t textio_stream_p = {
    .read = file_read,
    .write = file_write,
    .is_text = true,
};

const mp_obj_type_t microbit_textio_type = {
    { &mp_type_type },
    .name = MP_QSTR_TextIO,
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
    .stream_p = &textio_stream_p,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_textio_locals_dict,
};

static file_descriptor_obj *microbit_file_descriptor_new(uint8_t start_chunk, bool write, bool binary) {
    file_descriptor_obj *res = m_new_obj(file_descriptor_obj);
    if (binary) {
        res->base.type = &microbit_bytesio_type;
    } else {
        res->base.type = &microbit_textio_type;
    }
    res->start_chunk = start_chunk;
    res->seek_chunk = start_chunk;
    res->seek_offset = file_system_chunks[start_chunk].header.name_len+3;
    res->writable = write;
    res->open = true;
    res->binary = binary;
    if (write) {
        res->end_chunk = 255;
        res->end_offset = 255;
    } else {
        res->end_chunk = file_system_chunks[start_chunk].header.end_chunk;
        res->end_offset = file_system_chunks[start_chunk].header.end_offset;
    }
    return res;
}

static mp_obj_t microbit_file_list(void) {
    mp_obj_t res = mp_obj_new_list(0, NULL);
    for (uint8_t index = 1; index <= CHUNKS_IN_FILE_SYSTEM; index++) {
        if (file_system_chunks[index].marker == FILE_START) {
            mp_obj_t name = mp_obj_new_str(&file_system_chunks[index].header.filename[0], file_system_chunks[index].header.name_len, false);
            mp_obj_list_append(res, name);
        }
    }
    return res;
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_file_list_obj, microbit_file_list);

static mp_obj_t microbit_file_size(mp_obj_t filename) {
    mp_uint_t name_len;
    const char *name = mp_obj_str_get_data(filename, &name_len);
    uint8_t chunk = microbit_find_file(name, name_len);
    if (chunk == 255) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "file not found"));
    }
    mp_uint_t len = 0;
    uint8_t end_chunk = file_system_chunks[chunk].header.end_chunk;
    uint8_t end_offset = file_system_chunks[chunk].header.end_offset;
    uint8_t offset = file_system_chunks[chunk].header.name_len+3;
    while (chunk != end_chunk) {
        len += DATA_PER_CHUNK - offset;
        chunk = file_system_chunks[chunk].next_chunk;
        offset = 0;
    }
    len += end_offset - offset;
    return mp_obj_new_int(len);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_file_size_obj, microbit_file_size);

static const mp_map_elem_t _globals_table[] = {
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

static mp_uint_t file_read_byte(file_descriptor_obj *fd) {
    if (fd->seek_offset == fd->end_offset && fd->seek_chunk == fd->end_chunk)
        return (mp_uint_t)-1;
    mp_uint_t res = file_system_chunks[fd->seek_chunk].data[fd->seek_offset];
    advance(fd, 1, false);
    return res;
}

mp_lexer_t *microbit_file_lexer(qstr src_name, file_descriptor_obj *fd) {
    return mp_lexer_new(src_name, fd, (mp_lexer_stream_next_byte_t)file_read_byte, (mp_lexer_stream_close_t)microbit_file_close);
}
