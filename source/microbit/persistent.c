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

#include "py/nlr.h"
#include "py/obj.h"
#include "py/gc.h"
#include "modfile.h"
#include "lib/ticker.h"

#define DEBUG_PERSISTENT 0
#if DEBUG_PERSISTENT
#define DEBUG(s) printf s
#else
#define DEBUG(s) (void)0
#endif

inline void persistent_write_byte_unchecked(const uint8_t *dest, const uint8_t val) {
#if DEBUG_PERSISTENT
    if (((~(*dest)) & val) != 0) {
        DEBUG(("PERSISTENCE DEBUG: ERROR: Unchecked write of byte %u to %x which contains %u\r\n", val, dest, *dest));
    }
#endif
    DEBUG(("PERSISTENCE DEBUG: Write unchecked byte %u to %x, previous value %u\r\n", val, dest, *dest));
    nrf_nvmc_write_byte((uint32_t)dest, val);
    DEBUG(("PERSISTENCE DEBUG: New value %u\r\n", *dest));
}

void persistent_write_unchecked(const void *dest, const void *src, uint32_t len) {
    DEBUG(("PERSISTENCE DEBUG: Write unchecked %lu bytes from %x to %x\r\n", len, src, dest));
    int8_t *address = (int8_t *)dest;
    int8_t *data = (int8_t *)src;
    // Writing to flash will stop the CPU, so we stop the ticker to minimise odd behaviour.
    ticker_stop();
    // Aligned word writes are over 4 times as fast per byte, so use those if we can.
    if ((((uint32_t)address) & 3) == 0 && (((uint32_t)data) & 3) == 0 && len >= 4) {
        nrf_nvmc_write_words((uint32_t)address, (const uint32_t *)data, len>>2);
        address += (len>>2)<<2;
        data += (len>>2)<<2;
        len &= 3;
    }
    if (len) {
        nrf_nvmc_write_bytes((uint32_t)address, (const uint8_t *)data, len);
    }
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy);
    ticker_start();
}

static inline bool can_write(const int8_t *dest, const int8_t *src, uint32_t len) {
    const int8_t *end = dest + len;
    while (dest < end) {
        if (((~(*dest)) & *src) != 0) {
            DEBUG(("PERSISTENCE DEBUG: %d bytes from %x need to be erased\r\n", len, dest));
            return false;
        }
        dest++;
        src++;
    }
    DEBUG(("PERSISTENCE DEBUG: %d bytes from %x do not need to be erased\r\n", len, dest));
    return true;
}

STATIC void erase_page(const void *page) {
    DEBUG(("PERSISTENCE DEBUG: Erasing page %x\r\n", page));
    nrf_nvmc_page_erase((uint32_t)page);
}

bool is_persistent_page_aligned(const void *ptr) {
    return (((uint32_t)ptr) & (persistent_page_size()-1)) == 0;
}

void persistent_erase(const void *dst, uint32_t len) {
    DEBUG(("PERSISTENCE DEBUG: Erase persistent %d bytes at %x\r\n", len, dst));
    const int8_t *dest = dst;
    const int8_t *end_erase = dest+len;
    const int8_t *addr = dest;
    const uint32_t page_size = persistent_page_size();
    if (is_persistent_page_aligned(dst) && len == page_size) {
        erase_page(dst);
        return;
    }
    int8_t *page = (void *)(((uint32_t)dest)&(-page_size));
    int8_t *tmp_storage = NULL;
    while (addr < end_erase) {
        int8_t *next_page = page + page_size;
        uint32_t in_page = min(end_erase-addr, next_page-dest);
        if (in_page < page_size) {
            if (tmp_storage == NULL) {
                tmp_storage = gc_alloc(page_size, false);
            }
            memcpy(tmp_storage, page, page_size);
            memset(tmp_storage+(dest-page), -1, in_page);
        }
        erase_page(page);
        if (in_page < page_size) {
            persistent_write_unchecked(page, tmp_storage, page_size);
        }
        addr += in_page;
    }
}

void persistent_write(const void *dst, const void *src, uint32_t len) {
    DEBUG(("PERSISTENCE DEBUG: Write persistent %d bytes from %x to %x\r\n", len, src, dst));
    const int8_t *dest = dst;
    const int8_t *addr = src;
    const int8_t *end_data = src+len;
    const uint32_t page_size = persistent_page_size();
    int8_t *page = (void *)(((uint32_t)dest)&(-page_size));
    int8_t *tmp_storage = NULL;
    while (addr < end_data) {
        int8_t *next_page = page + page_size;
        uint32_t data_in_page = min(end_data-addr, next_page-dest);
        if (can_write(dest, addr, data_in_page)) {
            persistent_write_unchecked(dest, addr, data_in_page);
        } else {
            if (tmp_storage == NULL) {
                tmp_storage = gc_alloc(page_size, false);
            }
            memcpy(tmp_storage, page, page_size);
            memcpy(tmp_storage+(dest-page), addr, data_in_page);
            erase_page(page);
            persistent_write_unchecked(page, tmp_storage, page_size);
        }
        dest = page = next_page;
        addr += data_in_page;
    }
    if (tmp_storage) {
        gc_free(tmp_storage);
    }
}

void persistent_write_byte(const uint8_t *dest, const uint8_t val) {
    DEBUG(("PERSISTENCE DEBUG: Write persistent byte %d to %x\r\n", val, dest));
    if (((~(*dest)) & val) == 0) {
        nrf_nvmc_write_byte((uint32_t)dest, val);
    } else {
        persistent_write(dest, &val, 1);
    }
}

#define FIVE(x) x,x,x,x,x
#define TWENTY(x) FIVE(x),FIVE(x),FIVE(x),FIVE(x)
#define ONE_HUNDRED(x) TWENTY(x),TWENTY(x),TWENTY(x),TWENTY(x),TWENTY(x)

#define CHUNK(x) ONE_HUNDRED(x),TWENTY(x),FIVE(x),x,x,x
#define FIVE_CHUNKS(x) CHUNK(x),CHUNK(x),CHUNK(x),CHUNK(x),CHUNK(x)
#define TWENTY_CHUNKS(x) FIVE_CHUNKS(x),FIVE_CHUNKS(x),FIVE_CHUNKS(x),FIVE_CHUNKS(x)
#define ONE_SIXTY_CHUNKS(x) TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x),TWENTY_CHUNKS(x)

const int8_t file_system_data[] __attribute__ ((aligned (1024))) = { ONE_SIXTY_CHUNKS(-1) };

STATIC_ASSERT((sizeof(file_system_data) == CHUNKS_IN_FILE_SYSTEM*CHUNK_SIZE));
