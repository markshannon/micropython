#include "microbitobj.h"
#include "PinNames.h"
#include "modmicrobit.h"

#include "MicroBitPin.h"
#include "microbitpin.h"

#ifdef CALLIOPE
/* Calliope */
const microbit_pin_obj_t microbit_p0_obj = {{&microbit_touch_pin_type}, 0, p0, MODE_UNUSED };
const microbit_pin_obj_t microbit_p1_obj = {{&microbit_touch_pin_type}, 1, p1, MODE_UNUSED};
const microbit_pin_obj_t microbit_p2_obj = {{&microbit_touch_pin_type}, 2, p2, MODE_UNUSED};
const microbit_pin_obj_t microbit_p3_obj = {{&microbit_ad_pin_type},   3,  MICROBIT_PIN_P3, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p4_obj = {{&microbit_ad_pin_type},   4,  MICROBIT_PIN_P4, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p5_obj = {{&microbit_dig_pin_type},  5,  MICROBIT_PIN_P5};
const microbit_pin_obj_t microbit_p6_obj = {{&microbit_dig_pin_type},  6,  MICROBIT_PIN_P6};
const microbit_pin_obj_t microbit_p7_obj = {{&microbit_dig_pin_type},  7,  MICROBIT_PIN_P7};
const microbit_pin_obj_t microbit_p8_obj = {{&microbit_dig_pin_type},  8,  MICROBIT_PIN_P8};
const microbit_pin_obj_t microbit_p9_obj = {{&microbit_dig_pin_type},  9,  MICROBIT_PIN_P9};
const microbit_pin_obj_t microbit_p10_obj = {{&microbit_ad_pin_type},  10, MICROBIT_PIN_P10};
const microbit_pin_obj_t microbit_p11_obj = {{&microbit_dig_pin_type}, 11, MICROBIT_PIN_P11};
const microbit_pin_obj_t microbit_p12_obj = {{&microbit_dig_pin_type}, 12, MICROBIT_PIN_P12};
const microbit_pin_obj_t microbit_p13_obj = {{&microbit_dig_pin_type}, 13, MICROBIT_PIN_P13};
const microbit_pin_obj_t microbit_p14_obj = {{&microbit_dig_pin_type}, 14, MICROBIT_PIN_P14};
const microbit_pin_obj_t microbit_p15_obj = {{&microbit_dig_pin_type}, 15, MICROBIT_PIN_P15};
const microbit_pin_obj_t microbit_p16_obj = {{&microbit_dig_pin_type}, 16, MICROBIT_PIN_P16};
const microbit_pin_obj_t microbit_p19_obj = {{&microbit_dig_pin_type}, 19, p19};
const microbit_pin_obj_t microbit_p20_obj = {{&microbit_dig_pin_type}, 20, p20};
const microbit_pin_obj_t microbit_p28_obj = {{&microbit_dig_pin_type}, 28, p28};
const microbit_pin_obj_t microbit_p29_obj = {{&microbit_dig_pin_type}, 29, p29};
const microbit_pin_obj_t microbit_p30_obj = {{&microbit_dig_pin_type}, 30, p30, MODE_UNUSED};


const microbit_hardware_t microbit_hardware = {
    .name = "Calliope-mini",
    .default_scl_pin = p19,
    .default_sda_pin = p20,
    .default_audio_pin = &microbit_p29_obj,
    .button_pins = { &microbit_p11_obj, &microbit_p5_obj }
};

#else

/* Standard microbit */
const microbit_pin_obj_t microbit_p0_obj = {{&microbit_touch_pin_type}, 0, MICROBIT_PIN_P0, MODE_UNUSED};
const microbit_pin_obj_t microbit_p1_obj = {{&microbit_touch_pin_type}, 1, MICROBIT_PIN_P1, MODE_UNUSED};
const microbit_pin_obj_t microbit_p2_obj = {{&microbit_touch_pin_type}, 2, MICROBIT_PIN_P2, MODE_UNUSED};
const microbit_pin_obj_t microbit_p3_obj = {{&microbit_ad_pin_type},   3,  MICROBIT_PIN_P3, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p4_obj = {{&microbit_ad_pin_type},   4,  MICROBIT_PIN_P4, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p5_obj = {{&microbit_dig_pin_type},  5,  MICROBIT_PIN_P5, MODE_BUTTON};
const microbit_pin_obj_t microbit_p6_obj = {{&microbit_dig_pin_type},  6,  MICROBIT_PIN_P6, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p7_obj = {{&microbit_dig_pin_type},  7,  MICROBIT_PIN_P7, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p8_obj = {{&microbit_dig_pin_type},  8,  MICROBIT_PIN_P8, MODE_UNUSED};
const microbit_pin_obj_t microbit_p9_obj = {{&microbit_dig_pin_type},  9,  MICROBIT_PIN_P9, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p10_obj = {{&microbit_ad_pin_type},  10, MICROBIT_PIN_P10, MODE_DISPLAY};
const microbit_pin_obj_t microbit_p11_obj = {{&microbit_dig_pin_type}, 11, MICROBIT_PIN_P11, MODE_BUTTON};
const microbit_pin_obj_t microbit_p12_obj = {{&microbit_dig_pin_type}, 12, MICROBIT_PIN_P12, MODE_UNUSED};
const microbit_pin_obj_t microbit_p13_obj = {{&microbit_dig_pin_type}, 13, MICROBIT_PIN_P13, MODE_SPI};
const microbit_pin_obj_t microbit_p14_obj = {{&microbit_dig_pin_type}, 14, MICROBIT_PIN_P14, MODE_SPI};
const microbit_pin_obj_t microbit_p15_obj = {{&microbit_dig_pin_type}, 15, MICROBIT_PIN_P15, MODE_SPI};
const microbit_pin_obj_t microbit_p16_obj = {{&microbit_dig_pin_type}, 16, MICROBIT_PIN_P16, MODE_UNUSED};
const microbit_pin_obj_t microbit_p19_obj = {{&microbit_dig_pin_type}, 19, MICROBIT_PIN_P19, MODE_I2C};
const microbit_pin_obj_t microbit_p20_obj = {{&microbit_dig_pin_type}, 20, MICROBIT_PIN_P20, MODE_I2C};

const microbit_hardware_t microbit_hardware = {
    .name = "micro:bit",
    .default_scl_pin = p0,
    .default_sda_pin = p30,
    .default_audio_pin = &microbit_p0_obj,
    .button_pins = { &microbit_p5_obj, &microbit_p11_obj }
};

#endif
