#pragma once

#include <stdint.h>
#include "asm_wrappers.h"
#include "i8254.h"

#define SPEAKER_PORT    0x61

// frequency rates to divide PIT channel 2 default rate of 1193182MHz by to get a note
typedef enum {
    A4      = 2711,
} NOTE_FREQS;

void speaker_enable();
void speaker_disable();
void play_note(const NOTE_FREQS note, uint32_t ms_duration);
void rest(uint32_t ms_duration);