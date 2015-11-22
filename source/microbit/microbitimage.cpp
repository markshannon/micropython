/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien George, Mark Shannon
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
#include "microbitobj.h"

extern "C" {

#include "py/runtime.h"
#include "modmicrobit.h"
#include "microbitimage.h"
#include "py/runtime0.h"

const monochrome_5by5_t microbit_blank_image = {
    { &microbit_image_type },
    1, 0, 0, 0,
    { 0, 0, 0 }
};

STATIC void microbit_image_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_printf(print, "Image(");
    if (kind == PRINT_STR)
        mp_printf(print, "\n    ");
    mp_printf(print, "'");
    for (int y = 0; y < self->height(); ++y) {
        for (int x = 0; x < self->width(); ++x) {
            mp_printf(print, "%c", " 123456789"[self->getPixelValue(x, y)]);
        }
        mp_printf(print, "\\n");
        if (kind == PRINT_STR && y < self->height()-1)
            mp_printf(print, "'\n    '");
    }
    mp_printf(print, "'");
    if (kind == PRINT_STR)
        mp_printf(print, "\n");
    mp_printf(print, ")");
}

uint8_t monochrome_5by5_t::getPixelValue(mp_int_t x, mp_int_t y) {
    unsigned int index = y*5+x;
    if (index == 24) 
        return this->pixel44;
    return (this->bits24[index>>3] >> (index&7))&1;
}

uint8_t greyscale_t::getPixelValue(mp_int_t x, mp_int_t y) {
    unsigned int index = y*this->width+x;
    unsigned int shift = ((index<<2)&4);
    return (this->byte_data[index>>1] >> shift)&15;
}

void greyscale_t::setPixelValue(mp_int_t x, mp_int_t y, mp_int_t val) {
    unsigned int index = y*this->width+x;
    unsigned int shift = ((index<<2)&4);
    uint8_t mask = 240 >> shift;
    this->byte_data[index>>1] = (this->byte_data[index>>1] & mask) | (val << shift);
}

void greyscale_t::clear() {
    memset(&this->byte_data, 0, (this->width*this->height+1)>>1);
}

uint8_t microbit_image_obj_t::getPixelValue(mp_int_t x, mp_int_t y) {
    if (this->base.five)
        return this->monochrome_5by5.getPixelValue(x, y)*MAX_BRIGHTNESS;
    else
        return this->greyscale.getPixelValue(x, y);
}

mp_int_t microbit_image_obj_t::width() {
    if (this->base.five)
        return 5;
    else
        return this->greyscale.width;
}

mp_int_t microbit_image_obj_t::height() {
    if (this->base.five)
        return 5;
    else
        return this->greyscale.height;
}

STATIC greyscale_t *greyscale_new(mp_int_t w, mp_int_t h) {
    greyscale_t *result = m_new_obj_var(greyscale_t, uint8_t, (w*h+1)>>1);
    result->base.type = &microbit_image_type;
    result->five = 0;
    result->width = w;
    result->height = h;
    return result;
}

greyscale_t *microbit_image_obj_t::copy() {
    mp_int_t w = this->width();
    mp_int_t h = this->height();
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t y = 0; y < h; y++) {
        for (mp_int_t x = 0; x < w; ++x) {
            result->setPixelValue(x,y, this->getPixelValue(x,y));
        }
    }
    return result;
}

greyscale_t *microbit_image_obj_t::invert() {
    mp_int_t w = this->width();
    mp_int_t h = this->height();
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t y = 0; y < h; y++) {
        for (mp_int_t x = 0; x < w; ++x) {
            result->setPixelValue(x,y, MAX_BRIGHTNESS - this->getPixelValue(x,y));
        }
    }
    return result;
}

greyscale_t *microbit_image_obj_t::shiftLeft(mp_int_t n) {
    mp_int_t w = this->width();
    mp_int_t h = this->height();
    n = max(n, -w);
    n = min(n, w);
    mp_int_t src_start = max(n, 0);
    mp_int_t src_end = min(w+n,w);
    mp_int_t dest = max(0,-n);
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t x = 0; x < dest; ++x) {
        for (mp_int_t y = 0; y < h; y++) {
            result->setPixelValue(x, y, 0);
        }
    }
    for (mp_int_t x = src_start; x < src_end; ++x) {
        for (mp_int_t y = 0; y < h; y++) {
             result->setPixelValue(dest, y, this->getPixelValue(x, y));
        }
        ++dest;
    }
    for (mp_int_t x = dest; x < w; ++x) {
        for (mp_int_t y = 0; y < h; y++) {
            result->setPixelValue(x, y, 0);
        }
    }
    return result;
}


greyscale_t *microbit_image_obj_t::shiftUp(mp_int_t n) {
    mp_int_t w = this->width();
    mp_int_t h = this->height();
    n = max(n, -h);
    n = min(n, h);
    mp_int_t src_start = max(n, 0);
    mp_int_t src_end = min(h+n,h);
    mp_int_t dest = max(0,-n);
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t y = 0; y < dest; ++y) {
        for (mp_int_t x = 0; x < w; x++) {
            result->setPixelValue(x, y, 0);
        }
    }
    for (mp_int_t y = src_start; y < src_end; ++y) {
        for (mp_int_t x = 0; x < w; x++) {
             result->setPixelValue(x, dest, this->getPixelValue(x, y));
        }
        ++dest;
    }
    for (mp_int_t y = dest; y < h; ++y) {
        for (mp_int_t x = 0; x < w; x++) {
            result->setPixelValue(x, y, 0);
        }
    }
    return result;
}

STATIC microbit_image_obj_t *image_from_parsed_str(const char *s, mp_int_t len) {
    mp_int_t w = 0;
    mp_int_t h = 0;
    mp_int_t line_len = 0;
    greyscale_t *result;
    /*First pass -- Establish metadata */
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n') {
            w = max(line_len, w);
            line_len = 0;
            ++h;
        } else if (c == ',') {
            /* Ignore commas */
        } else if (c == ' ') {
            ++line_len;
        } else if ('c' >= '0' && c <= '9') {
            ++line_len;
        } else {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                "Unexpected character in Image definition."));
        }
    }
    if (line_len) {
        // Omitted trainling newline
        ++h;
        w = max(line_len, w); 
    }
    result = greyscale_new(w, h);
    mp_int_t x = 0;
    mp_int_t y = 0;
    /* Second pass -- Fill in data */
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n') {
            while (x < w) {
                result->setPixelValue(x, y, 0);
                x++;
            }
            ++y;
            x = 0;
        } else if (c == ',') {
            /* Ignore commas */
        } else if (c == ' ') {
            /* Treat spaces as 0 */
            result->setPixelValue(x, y, 0);
            ++x;
        } else if ('c' >= '0' && c <= '9') {
            result->setPixelValue(x, y, c - '0');
            ++x;
        }
    }
    if (y < h) {
        while (x < w) {
            result->setPixelValue(x, y, 0);
            x++;
        }
    }
    return (microbit_image_obj_t *)result;
}


STATIC mp_obj_t microbit_image_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args) {
    (void)type_in;
    mp_arg_check_num(n_args, n_kw, 0, 3, false);

    switch (n_args) {
        case 0: {
            return BLANK_IMAGE;
        }

        case 1: {
            if (MP_OBJ_IS_STR(args[0])) {
                // arg is a string object
                mp_uint_t len;
                const char *str = mp_obj_str_get_data(args[0], &len);
                // make image from string
                if (len == 1) {
                    /* For a single charater, return the font glyph */
                    return microbit_image_for_char(str[0]);
                } else {
                    /* Otherwise parse the image description string */
                    return image_from_parsed_str(str, len);
                }
            } else {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                    "Image(s) takes a string."));
            }
        }

        case 2:
        case 3: {
            mp_int_t w = mp_obj_get_int(args[0]);
            mp_int_t h = mp_obj_get_int(args[1]);
            greyscale_t *image = greyscale_new(w, h);
            if (n_args == 2) {
                image->clear();
            } else {
                mp_buffer_info_t bufinfo;
                mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);

                if (w < 0 || h < 0 || (size_t)(w * h) != bufinfo.len) {
                    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
                        "image data is incorrect size"));
                }
                mp_int_t i = 0;
                for (mp_int_t y = 0; y < h; y++) {
                    for (mp_int_t x = 0; x < w; ++x) {
                        uint8_t val = min(((const uint8_t*)bufinfo.buf)[i], MAX_BRIGHTNESS);
                        image->setPixelValue(x, y, val);
                        ++i;
                    }
                }
            }
            return image;
        }

        default: {
            nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError,
                "Image() takes 0 to 3 arguments"));
        }
    }
}

STATIC microbit_image_obj_t *image_crop(microbit_image_obj_t *img, mp_int_t x, mp_int_t y, mp_int_t w, mp_int_t h) {
    if (w < 0)
        w = 0;
    if (h < 0)
        h = 0;
    greyscale_t *result = greyscale_new(w, h);
    mp_int_t intersect_x0 = max(0, x);   
    mp_int_t intersect_y0 = max(0, y);
    mp_int_t intersect_x1 = min(img->width(), x+w);
    mp_int_t intersect_y1 = min(img->height(), y+h);
    /* If the cropped image is larger than the intersection then
     * make sure that all other pixels are set to 0 */
    if (w > intersect_x1 - intersect_x0 || h > intersect_y1 - intersect_y0) {
        result->clear();
    }
    for (int i = intersect_x0; i < intersect_x1; ++i) {
        for (int j = intersect_y0; j < intersect_y1; ++j) {
            int val = img->getPixelValue(i, j);
            result->setPixelValue(i-x, j-y, val);
        }
    }
    return (microbit_image_obj_t *)result;
}

mp_obj_t microbit_image_width(mp_obj_t self_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    return MP_OBJ_NEW_SMALL_INT(self->width());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_image_width_obj, microbit_image_width);

mp_obj_t microbit_image_height(mp_obj_t self_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    return MP_OBJ_NEW_SMALL_INT(self->height());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_image_height_obj, microbit_image_height);

mp_obj_t microbit_image_get_pixel(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_int_t x = mp_obj_get_int(x_in);
    mp_int_t y = mp_obj_get_int(y_in);
    if (x < 0 || y < 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
            "index cannot be negative"));
    }
    if (x < self->width() && y < self->height()) {
        return MP_OBJ_NEW_SMALL_INT(self->getPixelValue(x, y));
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "index too large"));
}
MP_DEFINE_CONST_FUN_OBJ_3(microbit_image_get_pixel_obj, microbit_image_get_pixel);

mp_obj_t microbit_image_set_pixel(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    microbit_image_obj_t *self = (microbit_image_obj_t*)args[0];
    if (self->base.five) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_TypeError, "This image cannot be modified. Try copying it first."));
    }
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    if (x < 0 || y < 0) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError,
            "index cannot be negative"));
    }
    mp_int_t bright = mp_obj_get_int(args[3]);
    if (bright < 0 || bright > MAX_BRIGHTNESS) 
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "brightness out of bounds."));
    if (x < self->width() && y < self->height()) {
        self->greyscale.setPixelValue(x, y, bright);
        return mp_const_none;
    }
    nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "index too large"));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_image_set_pixel_obj, 4, 4, microbit_image_set_pixel);

mp_obj_t microbit_image_crop(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    microbit_image_obj_t *self = (microbit_image_obj_t*)args[0];
    mp_int_t x0 = mp_obj_get_int(args[1]);
    mp_int_t y0 = mp_obj_get_int(args[2]);
    mp_int_t x1 = mp_obj_get_int(args[3]);
    mp_int_t y1 = mp_obj_get_int(args[4]);
    return image_crop(self, x0, y0, x1, y1);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_image_crop_obj, 5, 5, microbit_image_crop);

mp_obj_t microbit_image_shift_left(mp_obj_t self_in, mp_obj_t n_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return self->shiftLeft(n);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_image_shift_left_obj, microbit_image_shift_left);

mp_obj_t microbit_image_shift_right(mp_obj_t self_in, mp_obj_t n_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return self->shiftLeft(-n);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_image_shift_right_obj, microbit_image_shift_right);

mp_obj_t microbit_image_shift_up(mp_obj_t self_in, mp_obj_t n_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return self->shiftUp(n);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_image_shift_up_obj, microbit_image_shift_up);

mp_obj_t microbit_image_shift_down(mp_obj_t self_in, mp_obj_t n_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    mp_int_t n = mp_obj_get_int(n_in);
    return self->shiftUp(-n);
}
MP_DEFINE_CONST_FUN_OBJ_2(microbit_image_shift_down_obj, microbit_image_shift_down);

mp_obj_t microbit_image_copy(mp_obj_t self_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    return self->copy();
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_image_copy_obj, microbit_image_copy);

mp_obj_t microbit_image_invert(mp_obj_t self_in) {
    microbit_image_obj_t *self = (microbit_image_obj_t*)self_in;
    return self->invert();
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_image_invert_obj, microbit_image_invert);

mp_obj_t microbit_image_slice_func(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    microbit_image_obj_t *self = (microbit_image_obj_t*)args[0];
    mp_int_t start = mp_obj_get_int(args[1]);
    mp_int_t width = mp_obj_get_int(args[2]);
    mp_int_t stride = mp_obj_get_int(args[3]);
    return microbit_image_slice(self, start,  width, stride);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_image_slice_obj, 4, 4, microbit_image_slice_func);


STATIC const mp_map_elem_t microbit_image_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_width), (mp_obj_t)&microbit_image_width_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_height), (mp_obj_t)&microbit_image_height_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_pixel), (mp_obj_t)&microbit_image_get_pixel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_pixel), (mp_obj_t)&microbit_image_set_pixel_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shift_left), (mp_obj_t)&microbit_image_shift_left_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shift_right), (mp_obj_t)&microbit_image_shift_right_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shift_up), (mp_obj_t)&microbit_image_shift_up_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shift_down), (mp_obj_t)&microbit_image_shift_down_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_copy), (mp_obj_t)&microbit_image_copy_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_crop), (mp_obj_t)&microbit_image_crop_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_invert), (mp_obj_t)&microbit_image_invert_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_slice), (mp_obj_t)&microbit_image_slice_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_HEART), (mp_obj_t)&microbit_const_image_heart_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_HEART_SMALL), (mp_obj_t)&microbit_const_image_heart_small_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_HAPPY), (mp_obj_t)&microbit_const_image_happy_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SMILE), (mp_obj_t)&microbit_const_image_smile_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SAD), (mp_obj_t)&microbit_const_image_sad_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CONFUSED), (mp_obj_t)&microbit_const_image_confused_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ANGRY), (mp_obj_t)&microbit_const_image_angry_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ASLEEP), (mp_obj_t)&microbit_const_image_asleep_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SURPRISED), (mp_obj_t)&microbit_const_image_surprised_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SILLY), (mp_obj_t)&microbit_const_image_silly_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_FABULOUS), (mp_obj_t)&microbit_const_image_fabulous_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MEH), (mp_obj_t)&microbit_const_image_meh_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_YES), (mp_obj_t)&microbit_const_image_yes_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_NO), (mp_obj_t)&microbit_const_image_no_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK12), (mp_obj_t)&microbit_const_image_clock12_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK1), (mp_obj_t)&microbit_const_image_clock1_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK2), (mp_obj_t)&microbit_const_image_clock2_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK3), (mp_obj_t)&microbit_const_image_clock3_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK4), (mp_obj_t)&microbit_const_image_clock4_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK5), (mp_obj_t)&microbit_const_image_clock5_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK6), (mp_obj_t)&microbit_const_image_clock6_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK7), (mp_obj_t)&microbit_const_image_clock7_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK8), (mp_obj_t)&microbit_const_image_clock8_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK9), (mp_obj_t)&microbit_const_image_clock9_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK10), (mp_obj_t)&microbit_const_image_clock10_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CLOCK11), (mp_obj_t)&microbit_const_image_clock11_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_N), (mp_obj_t)&microbit_const_image_arrow_n_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_NE), (mp_obj_t)&microbit_const_image_arrow_ne_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_E), (mp_obj_t)&microbit_const_image_arrow_e_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_SE), (mp_obj_t)&microbit_const_image_arrow_se_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_S), (mp_obj_t)&microbit_const_image_arrow_s_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_SW), (mp_obj_t)&microbit_const_image_arrow_sw_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_W), (mp_obj_t)&microbit_const_image_arrow_w_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_ARROW_NW), (mp_obj_t)&microbit_const_image_arrow_nw_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TRIANGLE), (mp_obj_t)&microbit_const_image_triangle_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TRIANGLE_LEFT), (mp_obj_t)&microbit_const_image_triangle_left_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_CHESSBOARD), (mp_obj_t)&microbit_const_image_chessboard_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DIAMOND), (mp_obj_t)&microbit_const_image_diamond_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_DIAMOND_SMALL), (mp_obj_t)&microbit_const_image_diamond_small_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SQUARE), (mp_obj_t)&microbit_const_image_square_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SQUARE_SMALL), (mp_obj_t)&microbit_const_image_square_small_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RABBIT), (mp_obj_t)&microbit_const_image_rabbit },
    { MP_OBJ_NEW_QSTR(MP_QSTR_COW), (mp_obj_t)&microbit_const_image_cow },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MUSIC_CROTCHET), (mp_obj_t)&microbit_const_image_music_crotchet_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MUSIC_QUAVERS), (mp_obj_t)&microbit_const_image_music_quavers_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PITCHFORK), (mp_obj_t)&microbit_const_image_pitchfork_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_XMAS), (mp_obj_t)&microbit_const_image_xmas_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PACMAN), (mp_obj_t)&microbit_const_image_pacman_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TARGET), (mp_obj_t)&microbit_const_image_target_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_image_locals_dict, microbit_image_locals_dict_table);


STATIC const unsigned char *get_font_data_from_char(char c) {
    MicroBitFont font = uBit.display.getFont();
    if (c < MICROBIT_FONT_ASCII_START || c > font.asciiEnd) {
        c = '?';
    }
    /* The following logic belongs in MicroBitFont */
    int offset = (c-MICROBIT_FONT_ASCII_START) * 5;
    return font.characters + offset;
}

STATIC mp_int_t get_pixel_from_font_data(const unsigned char *data, int x, int y) {
    /* The following logic belongs in MicroBitFont */
    return ((data[y]>>(4-x))&1);
}

microbit_image_obj_t *microbit_image_for_char(char c) {
    const unsigned char *data = get_font_data_from_char(c);
    greyscale_t *result = greyscale_new(5,5);
    for (int x = 0; x < 5; ++x) {
        for (int y = 0; y < 5; ++y) {
            result->setPixelValue(x, y, get_pixel_from_font_data(data, x, y)*MAX_BRIGHTNESS);
        }
    }
    return (microbit_image_obj_t *)result;
}

microbit_image_obj_t *microbit_image_dim(microbit_image_obj_t *lhs, mp_float_t fval) {
    if (fval < 0) 
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Brightness multiplier must not be negative."));
    greyscale_t *result = greyscale_new(lhs->width(), lhs->height());
    for (int x = 0; x < lhs->width(); ++x) {
        for (int y = 0; y < lhs->width(); ++y) {
            int val = min((int)lhs->getPixelValue(x,y)*fval+0.5, MAX_BRIGHTNESS);
            result->setPixelValue(x, y, val);
        }
    }
    return (microbit_image_obj_t *)result;
}

microbit_image_obj_t *microbit_image_sum(microbit_image_obj_t *lhs, microbit_image_obj_t *rhs, bool add) {
    mp_int_t h = lhs->height();
    mp_int_t w = lhs->width();
    if (rhs->height() != h || lhs->width() != w) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_ValueError, "Images must be the same size."));
    }
    greyscale_t *result = greyscale_new(w, h);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int val;
            int lval = lhs->getPixelValue(x,y);
            int rval = rhs->getPixelValue(x,y);
            if (add) 
                val = min(lval + rval, MAX_BRIGHTNESS);
            else
                val = max(0, lval - rval);
            result->setPixelValue(x, y, val);
        }
    }
    return (microbit_image_obj_t *)result;
}                      
                                   
STATIC mp_obj_t image_binary_op(mp_uint_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    if (mp_obj_get_type(lhs_in) != &microbit_image_type) {
        return MP_OBJ_NULL; // op not supported
    }
    microbit_image_obj_t *lhs = (microbit_image_obj_t *)lhs_in;
    switch(op) {
    case MP_BINARY_OP_ADD:
    case MP_BINARY_OP_SUBTRACT:
        break;
    case MP_BINARY_OP_MULTIPLY:
        return microbit_image_dim(lhs, mp_obj_get_float(rhs_in));
    case MP_BINARY_OP_TRUE_DIVIDE:
        return microbit_image_dim(lhs, 1.0/mp_obj_get_float(rhs_in));
    default:
        return MP_OBJ_NULL; // op not supported
    }
    if (mp_obj_get_type(rhs_in) != &microbit_image_type) {
        return MP_OBJ_NULL; // op not supported
    }
    return microbit_image_sum(lhs, (microbit_image_obj_t *)rhs_in, op == MP_BINARY_OP_ADD);
}


const mp_obj_type_t microbit_image_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitImage,
    .print = microbit_image_print,
    .make_new = microbit_image_make_new,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = image_binary_op,
    .attr = NULL,
    .subscr = NULL,
    .getiter = NULL,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    /* .locals_dict = */ (mp_obj_t)&microbit_image_locals_dict,
};


/* Image slices */

typedef struct _image_slice_t {
    mp_obj_base_t base; 
    microbit_image_obj_t *img;
    mp_int_t start;
    mp_int_t width;
    mp_int_t stride;
} image_slice_t;

typedef struct _image_slice_iterator_t {
    mp_obj_base_t base; 
    image_slice_t *slice;
    mp_int_t next_start;
} image_slice_iterator_t;

extern const mp_obj_type_t microbit_sliced_image_type;
extern const mp_obj_type_t microbit_sliced_image_iterator_type;

mp_obj_t microbit_image_slice(microbit_image_obj_t *img, mp_int_t start, mp_int_t width, mp_int_t stride) {
    image_slice_t *result = m_new_obj(image_slice_t);
    result->base.type = &microbit_sliced_image_type;
    result->img = img;
    result->start = start;
    result->width = width;
    result->stride = stride;
    return result;
}

STATIC mp_obj_t get_microbit_image_slice_iter(mp_obj_t o_in) {
    image_slice_t *slice = (image_slice_t *)o_in;
    image_slice_iterator_t *result = m_new_obj(image_slice_iterator_t);
    result->base.type = &microbit_sliced_image_iterator_type;
    result->slice = slice;
    result->next_start = slice->start;
    return result;
}

STATIC mp_obj_t microbit_image_slice_iter_next(mp_obj_t o_in) {
    image_slice_iterator_t *iter = (image_slice_iterator_t *)o_in;
    mp_obj_t result;
    microbit_image_obj_t *img = iter->slice->img;
    if (iter->slice->stride > 0 && iter->next_start >= img->width()) {
        return MP_OBJ_STOP_ITERATION;
    }
    if (iter->slice->stride < 0 && iter->next_start <= -iter->slice->width) {
        return MP_OBJ_STOP_ITERATION;
    }
    result = image_crop(img, iter->next_start, 0, iter->slice->width, img->height());
    iter->next_start += iter->slice->stride;
    return result;
}

const mp_obj_type_t microbit_sliced_image_type = {
    { &mp_type_type },
    .name = MP_QSTR_SlicedImage,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = get_microbit_image_slice_iter,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    MP_OBJ_NULL
};

const mp_obj_type_t microbit_sliced_image_iterator_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = microbit_image_slice_iter_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    MP_OBJ_NULL
};
 
typedef struct _scrolling_string_t {
    mp_obj_base_t base; 
    char const *str;
    mp_uint_t len;
    mp_obj_t ref;
    bool monospace;
} scrolling_string_t;

typedef struct _scrolling_string_iterator_t {
    mp_obj_base_t base; 
    mp_obj_t ref;
    microbit_image_obj_t *img;
    char const *next_char;
    char const *end;
    uint8_t offset;
    uint8_t offset_limit;
    bool monospace;
    char right;
} scrolling_string_iterator_t;

extern const mp_obj_type_t microbit_scrolling_string_type;
extern const mp_obj_type_t microbit_scrolling_string_iterator_type;

mp_obj_t scrolling_string_image_iterable(const char* str, mp_uint_t len, mp_obj_t ref, bool monospace) {
    scrolling_string_t *result = m_new_obj(scrolling_string_t);
    result->base.type = &microbit_scrolling_string_type;
    result->str = str;
    result->len = len;
    result->ref = ref;
    result->monospace = monospace;
    return result;
}

STATIC int font_column_non_blank(const unsigned char *font_data, unsigned int col) {
    for (int y = 0; y < 5; ++y) {
        if (get_pixel_from_font_data(font_data, col, y)) {
            return 1;
        }
    }
    return 0;
}

/* Not strictly the rightmost non-blank column, but the rightmost in columns 2,3 or 4. */
STATIC unsigned int rightmost_non_blank_column(const unsigned char *font_data) {
    if (font_column_non_blank(font_data, 4)) {
        return 4;
    }
    if (font_column_non_blank(font_data, 3)) {
        return 3;
    }
    return 2;
}

STATIC mp_obj_t get_microbit_scrolling_string_iter(mp_obj_t o_in) {
    scrolling_string_t *str = (scrolling_string_t *)o_in;
    scrolling_string_iterator_t *result = m_new_obj(scrolling_string_iterator_t);
    result->base.type = &microbit_scrolling_string_iterator_type;
    result->img = BLANK_IMAGE;
    result->offset = 0;
    result->next_char = str->str;
    result->ref = str->ref;
    result->monospace = str->monospace;
    result->end = result->next_char + str->len;
    if (str->len) {
        result->right = *result->next_char;
        if (result->monospace) {
            result->offset_limit = 5;
        } else {
            result->offset_limit = rightmost_non_blank_column(get_font_data_from_char(result->right)) + 1;
        }
    } else {
        result->right = ' ';
        result->offset_limit = 5;
    }
    return result;
}

STATIC mp_obj_t microbit_scrolling_string_iter_next(mp_obj_t o_in) {
    scrolling_string_iterator_t *iter = (scrolling_string_iterator_t *)o_in;
    if (iter->next_char == iter->end && iter->offset == 5) {
        return MP_OBJ_STOP_ITERATION;
    }
    greyscale_t *result = iter->img->shiftLeft(1);
    iter->img = (microbit_image_obj_t*)result;
    const unsigned char *font_data;
    if (iter->offset < iter->offset_limit) {
        font_data = get_font_data_from_char(iter->right);
        for (int y = 0; y < 5; ++y) {
            int pix = get_pixel_from_font_data(font_data, iter->offset, y)*MAX_BRIGHTNESS;
            result->setPixelValue(4, y, pix);
        }
    } else if (iter->offset == iter->offset_limit) {
        ++iter->next_char;
        if (iter->next_char == iter->end) {
            iter->right = ' ';
            iter->offset_limit = 5;
            iter->offset = 0;
        } else {
            iter->right = *iter->next_char;
            font_data = get_font_data_from_char(iter->right);
            if (iter->monospace) {
                iter->offset = -1;
                iter->offset_limit = 5;
            } else {
                iter->offset = -font_column_non_blank(font_data, 0);
                iter->offset_limit = rightmost_non_blank_column(font_data)+1;
            }
        }
    }
    ++iter->offset;
    return result;
}

const mp_obj_type_t microbit_scrolling_string_type = {
    { &mp_type_type },
    .name = MP_QSTR_ScrollingString,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = get_microbit_scrolling_string_iter,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    MP_OBJ_NULL
};

const mp_obj_type_t microbit_scrolling_string_iterator_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = microbit_scrolling_string_iter_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = MP_OBJ_NULL,
    MP_OBJ_NULL
};

}
