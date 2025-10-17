#include <stdint.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"

void crash_me();

void timer(Registers* regs) {
    printk(".");
}

extern uint8_t __bss_start;
extern uint8_t __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive) {
    memset(&__bss_start, 0, (uint16_t)((&__end) - (&__bss_start)));
    HAL_Init();
    printk("Hello world from splongleOS kernel\n");

    i686_IRQ_RegisterHandler(0, timer);

    // crash_me();

    end:
        for (;;);
}
