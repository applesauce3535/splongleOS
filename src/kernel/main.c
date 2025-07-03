#include <stdint.h>
#include "stdio.h"
#include "memory.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive) {
    memset(&__bss_start, 0, (&__end) - (&__bss_start));
    // vga buffer test
    volatile uint8_t* vga = (uint8_t*)0xB8000;
    for (int i = 0; i < 50; i++) {
        vga[i * 2] = 'A';
        vga[i * 2 + 1] = 0x0F;
    }
    // any stdio function seems to create an infinite loop where it cycles through colors
    // at the current vga location
    printf("Hi\n");

    for (int i = 0; i < 80; i++) {
        vga[i * 2] = 'B';
        vga[i * 2 + 1] = 0x0F;
    }
    end:
        for (;;);
}
