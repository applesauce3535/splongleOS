#include "irq.h"
#include "pic.h"
#include "io.h"
#include "stdio.h"
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20   // first 32 ISRs are CPU exceptions so start at offset 32 (0x20)

IRQHandler g_IRQHandlers[16];

void i686_IRQ_Handler(Registers* regs) {
    // determine which irq was called
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    // call corresponding irq handler
    if (g_IRQHandlers[irq] != NULL) {
        g_IRQHandlers[irq](regs);
    }
    else {
        printk("Unhandled IRQ %d\n", irq);
    }

    i686_PIC_SendEndOfInt(irq);
}

void i686_IRQ_Init() {
    i686_PIC_Config(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    // register ISR handlers for each of the 16 irq lines
    for (int i = 0;i < 16; ++i) {
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
    }

    // enable interrupts
    i686_EnableInts();
}
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler) {
    g_IRQHandlers[irq] = handler;
}