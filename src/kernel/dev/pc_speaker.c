/*
    functions for emulated (possibly real) pc speaker
*/

#include "pc_speaker.h"
#include "arch/i686/asm_wrappers.h"
#include "arch/i686/i8254.h"

void speaker_enable() {
    uint8_t temp = i686_inb(SPEAKER_PORT);
    i686_outb(SPEAKER_PORT, temp | 0x3);          // set first 2 bits to turn on speaker
}

void speaker_disable() {
    uint8_t temp = i686_inb(SPEAKER_PORT);
    i686_outb(SPEAKER_PORT, temp & 0xFC);         // clear first 2 bits to turn off speaker
}

void play_note(const NOTE_FREQS note, uint32_t ms_duration) {
    speaker_enable();
    set_PIT_channel_mode_frequency(2, 3, note);
    sleep(ms_duration);
    speaker_disable();
}

void rest(uint32_t ms_duration) {
    speaker_enable();
    set_PIT_channel_mode_frequency(2, 3, 40);
    sleep(ms_duration);
    speaker_disable();
}