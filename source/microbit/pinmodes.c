
#include "py/runtime.h"
#include "microbitpin.h"
#include "py/qstr.h"
#include "lib/pwm.h"

static void noop(const microbit_pin_obj_t *pin) {
    (void)pin;
}

void pinmode_error(const microbit_pin_obj_t *pin) {
    const microbit_pinmode_t *current_mode = microbit_pin_get_mode(pin);
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Pin %d in %q mode", pin->number, current_mode->name));
}

static void analog_release(const microbit_pin_obj_t *pin) {
    pwm_release(pin->name);
}

const microbit_pinmode_t microbit_pinmodes[] = {
    [MODE_UNUSED]        = { MP_QSTR_unused, noop },
    [MODE_WRITE_ANALOG]  = { MP_QSTR_write_analog, analog_release },
    [MODE_READ_DIGITAL]  = { MP_QSTR_read_digital, noop },
    [MODE_WRITE_DIGITAL] = { MP_QSTR_write_digital, noop },
    [MODE_DISPLAY]       = { MP_QSTR_display, pinmode_error },
    [MODE_BUTTON]        = { MP_QSTR_button, pinmode_error },
    [MODE_MUSIC]         = { MP_QSTR_music, pinmode_error },
    [MODE_AUDIO_PLAY]    = { MP_QSTR_audio, noop },
    [MODE_TOUCH]         = { MP_QSTR_touch, pinmode_error },
    [MODE_I2C]           = { MP_QSTR_i2c, pinmode_error },
    [MODE_SPI]           = { MP_QSTR_spi, pinmode_error }
};

