
#ifndef __MICROPY_INCLUDED_MICROBIT_BUTTON_H__
#define __MICROPY_INCLUDED_MICROBIT_BUTTON_H__

extern "C" {

#define MICROBIT_PIN_BUTTON_A                   P0_17
#define MICROBIT_PIN_BUTTON_B                   P0_26

#include "py/runtime.h"
#include "hal/DebouncedPin.h"

typedef struct _microbit_button_obj_t {
    mp_obj_base_t base;
    DebouncedPin pin;
    /* Stores pressed count in top 31 bits and was_pressed in the low bit */
    mp_uint_t pressed;

    void tick(void);

} microbit_button_obj_t;

extern microbit_button_obj_t microbit_button_a_obj;
extern microbit_button_obj_t microbit_button_b_obj;

}

#endif // __MICROPY_INCLUDED_MICROBIT_BUTTON_H__
