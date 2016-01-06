#ifndef __MICROPY_INCLUDED_MICROBIT_MUSIC_H__
#define __MICROPY_INCLUDED_MICROBIT_MUSIC_H__

extern "C" {

void microbit_music_tick(void);

struct MusicNote {
    int32_t period_us:14;
    int32_t octave_shift:5;
    int32_t duration:12;
};

MusicNote parse_note(const char *note_str, size_t note_len);

}

#endif // __MICROPY_INCLUDED_MICROBIT_MUSIC_H__
