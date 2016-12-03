/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George, 2016 Mark Shannon
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

extern "C" {

#include "lib/ticker.h"
#include "py/runtime.h"
#include "modmicrobit.h"
#include "py/mphal.h"
#include "microbitdisplay.h"
#include "microbitmatrix.h"

#include "py/runtime0.h"
//#include "py/runtime.h"

typedef struct _microbit_compass_obj_t {
    mp_obj_base_t base;
    MicroBitCompass *compass;
} microbit_compass_obj_t;

mp_obj_t microbit_compass_is_calibrated(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    return mp_obj_new_bool(self->compass->isCalibrated());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_is_calibrated_obj, microbit_compass_is_calibrated);

extern mp_obj_t microbit_accelerometer_get_x(mp_obj_t self_in);
extern mp_obj_t microbit_accelerometer_get_y(mp_obj_t self_in);

mp_obj_t microbit_compass_calibrate(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    struct Point
    {
        uint8_t x;
        uint8_t y;
        uint8_t on;
    };

    const int PERIMETER_POINTS = 12;
    const int PIXEL1_THRESHOLD = 200;
    const int PIXEL2_THRESHOLD = 800;

    mp_hal_delay_ms(100);

    microbit_matrix_obj_t *X = microbit_make_matrix(PERIMETER_POINTS, 4);
    Point perimeter[PERIMETER_POINTS] = {{1,0,0}, {2,0,0}, {3,0,0}, {4,1,0}, {4,2,0}, {4,3,0}, {3,4,0}, {2,4,0}, {1,4,0}, {0,3,0}, {0,2,0}, {0,1,0}};
    Point cursor = {2,2,0};
    // Firstly, we need to take over the display. Ensure all active animations are paused.
    microbit_display_scroll(&microbit_display_obj, "DRAW A CIRCLE");

start:
    int samples = 0;

    while(samples < PERIMETER_POINTS)
    {
        // update our model of the flash status of the user controlled pixel.
        cursor.on = (cursor.on + 1) % 4;

        // take a snapshot of the current accelerometer data.
        int x =  mp_obj_get_int(microbit_accelerometer_get_x((mp_obj_t)&microbit_accelerometer_obj));
        int y =  mp_obj_get_int(microbit_accelerometer_get_y((mp_obj_t)&microbit_accelerometer_obj));

        // Wait a little whie for the button state to stabilise (one scheduler tick).
        mp_hal_delay_ms(10);

        // Deterine the position of the user controlled pixel on the screen.
        if (x < -PIXEL2_THRESHOLD)
            cursor.x = 0;
        else if (x < -PIXEL1_THRESHOLD)
            cursor.x = 1;
        else if (x > PIXEL2_THRESHOLD)
            cursor.x = 4;
        else if (x > PIXEL1_THRESHOLD)
            cursor.x = 3;
        else
            cursor.x = 2;

        if (y < -PIXEL2_THRESHOLD)
            cursor.y = 0;
        else if (y < -PIXEL1_THRESHOLD)
            cursor.y = 1;
        else if (y > PIXEL2_THRESHOLD)
            cursor.y = 4;
        else if (y > PIXEL1_THRESHOLD)
            cursor.y = 3;
        else
            cursor.y = 2;

        microbit_image_obj_t *img = microbit_image_for_char(' ');


        // Turn on any pixels that have been visited.
        for (int i=0; i<PERIMETER_POINTS; i++) {
            if (perimeter[i].on) {
                img->greyscale.setPixelValue(perimeter[i].x, perimeter[i].y, 9);
            }
        }

        // Update the pixel at the users position.

        img->greyscale.setPixelValue(cursor.x, cursor.y, 9);

        // Update the buffer to the screen.
        microbit_display_show(&microbit_display_obj, img);

        // test if we need to update the state at the users position.
        for (int i=0; i<PERIMETER_POINTS; i++)
        {
            if (cursor.x == perimeter[i].x && cursor.y == perimeter[i].y && !perimeter[i].on)
            {
                // Record the sample data for later processing...
                microbit_matrix_set(X, samples, 0, self->compass->getX(RAW));
                microbit_matrix_set(X, samples, 1, self->compass->getY(RAW));
                microbit_matrix_set(X, samples, 2, self->compass->getZ(RAW));
                microbit_matrix_set(X, samples, 3, 1);

                // Record that this pixel has been visited.
                perimeter[i].on = 1;
                samples++;
            }
        }

        mp_hal_delay_ms(100);
    }

    // We have enough sample data to make a fairly accurate calibration.
    // We use a Least Mean Squares approximation, as detailed in Freescale application note AN2426.

    // Firstly, calculate the square of each sample.
    microbit_matrix_obj_t *Y = microbit_make_matrix(X->rows, 1);
    for (int i = 0; i < X->rows; i++)
    {
        float v = microbit_matrix_get(X, i, 0)*microbit_matrix_get(X, i, 0) +
                  microbit_matrix_get(X, i, 1)*microbit_matrix_get(X, i, 1) +
                  microbit_matrix_get(X, i, 2)*microbit_matrix_get(X, i, 2);
        microbit_matrix_set(Y, i, 0, v);
    }

    // Now perform a Least Squares Approximation.
    microbit_matrix_obj_t *T = microbit_matrix_transpose(X);
    microbit_matrix_obj_t *prod = microbit_matrix_mult(T, X);
    microbit_matrix_obj_t *Alpha = microbit_matrix_invert_maybe(prod);
    if (Alpha == NULL) {
        microbit_display_scroll(&microbit_display_obj, "TRY AGAIN");
        goto start;
    }
    microbit_matrix_obj_t *Gamma = microbit_matrix_mult(T, Y);
    microbit_matrix_obj_t *Beta = microbit_matrix_mult(Alpha, Gamma);


    // The result contains the approximate zero point of each axis, but doubled.
    // Halve each sample, and record this as the compass calibration data.
    CompassSample cal ((int)(microbit_matrix_get(Beta, 0, 0) / 2), (int)(microbit_matrix_get(Beta, 1, 0) / 2), (int)(microbit_matrix_get(Beta, 2, 0) / 2));
    self->compass->setCalibration(cal);

    // Show a smiley to indicate that we're done, and continue on with the user program.
    microbit_display_show(&microbit_display_obj, SMILE_IMAGE);
    mp_hal_delay_ms(1000);
    microbit_display_clear();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_calibrate_obj, microbit_compass_calibrate);

mp_obj_t microbit_compass_clear_calibration(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    self->compass->clearCalibration();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_clear_calibration_obj, microbit_compass_clear_calibration);

volatile bool compass_up_to_date = false;
volatile bool compass_updating = false;

static void update(microbit_compass_obj_t *self) {
    /* The only time it is possible for compass_updating to be true here
     * is if this is called in an interrupt when it is already updating in
     * the main execution thread. This is extremely unlikely, so we just
     * accept that a slightly out-of-date result will be returned
     */
    if (!compass_up_to_date && !compass_updating) {
        compass_updating = true;
        self->compass->idleTick();
        compass_updating = false;
        compass_up_to_date = true;
    }
}

mp_obj_t microbit_compass_heading(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    // Upon calling heading(), the DAL will automatically calibrate the compass
    // if it's not already calibrated.  Since we need to first enable the display
    // for calibration to work, we must check for non-calibration here and call
    // our own calibration function.
    if (!self->compass->isCalibrated()) {
        microbit_compass_calibrate(self_in);
    }
    update(self);
    return mp_obj_new_int(self->compass->heading());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_heading_obj, microbit_compass_heading);

mp_obj_t microbit_compass_get_x(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->compass->getX());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_x_obj, microbit_compass_get_x);

mp_obj_t microbit_compass_get_y(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->compass->getY());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_y_obj, microbit_compass_get_y);

mp_obj_t microbit_compass_get_z(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->compass->getZ());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_z_obj, microbit_compass_get_z);

mp_obj_t microbit_compass_get_field_strength(mp_obj_t self_in) {
    microbit_compass_obj_t *self = (microbit_compass_obj_t*)self_in;
    update(self);
    return mp_obj_new_int(self->compass->getFieldStrength());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_compass_get_field_strength_obj, microbit_compass_get_field_strength);

STATIC const mp_map_elem_t microbit_compass_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_heading), (mp_obj_t)&microbit_compass_heading_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_is_calibrated), (mp_obj_t)&microbit_compass_is_calibrated_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_calibrate), (mp_obj_t)&microbit_compass_calibrate_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_clear_calibration), (mp_obj_t)&microbit_compass_clear_calibration_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_x), (mp_obj_t)&microbit_compass_get_x_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_y), (mp_obj_t)&microbit_compass_get_y_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_z), (mp_obj_t)&microbit_compass_get_z_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_field_strength), (mp_obj_t)&microbit_compass_get_field_strength_obj },
};

STATIC MP_DEFINE_CONST_DICT(microbit_compass_locals_dict, microbit_compass_locals_dict_table);

STATIC const mp_obj_type_t microbit_compass_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitCompass,
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
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&microbit_compass_locals_dict,
};

const microbit_compass_obj_t microbit_compass_obj = {
    {&microbit_compass_type},
    .compass = &uBit.compass,
};

}
