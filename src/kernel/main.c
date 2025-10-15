#include <stdint.h>
#include "stdio.h"
#include "memory.h"

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive) {
    memset(&__bss_start, 0, (uint16_t)((&__end) - (&__bss_start)));
    printk("hello world from splongleOS kernel\n");
    end:
        for (;;);
}
