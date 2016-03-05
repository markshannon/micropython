
#ifndef __MICROPY_INCLUDED_MICROBIT_SOUND_H__
#define __MICROPY_INCLUDED_MICROBIT_SOUND_H__

#include "nrf.h"
#include "py/obj.h"
#include "py/runtime.h"

void sound_start(void);
void sound_stop(void);
mp_obj_t sound_init(void);
void sound_mute(void);
void sound_play_source(mp_obj_t src, bool wait);
mp_obj_t sound_set_pins(mp_obj_t pin0_obj, mp_obj_t pin1_obj);

#define LOG_SOUND_CHUNK_SIZE 5
#define SOUND_CHUNK_SIZE 32
#define SOUND_BUFFER_SIZE (SOUND_CHUNK_SIZE*2)
#define SOUND_CALLBACK_ID 0

typedef struct _microbit_sound_bytes_obj_t {
    mp_obj_base_t base;
    int8_t data[SOUND_CHUNK_SIZE];
} microbit_sound_bytes_obj_t;

extern const mp_obj_type_t microbit_sound_bytes_type;

microbit_sound_bytes_obj_t *new_microbit_sound_bytes(void);

/* Should return the new phase, which should be phase + phase_delta * SOUND_CHUNK_SIZE */
typedef int32_t (*oscillator_func_ptr)( microbit_sound_bytes_obj_t *frame, int32_t phase_delta, int32_t phase);

typedef struct _microbit_oscillator_obj_t {
    mp_obj_base_t base;
    oscillator_func_ptr oscillator ;
} microbit_oscillator_obj_t;

#endif // __MICROPY_INCLUDED_MICROBIT_SOUND_H__
