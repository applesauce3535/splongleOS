#include "include/irq.h"

IRQHandler g_IRQHandlers[16];
static const PICDriver* g_Driver = NULL;

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

    // don't need to send EOI because auto EOI enabled
}

void i686_IRQ_Init() {
    const PICDriver* drivers[] = {
        i8259_GetDriver()
    };

    // looking for a driver that exists
    for (int i = 0; i < SIZE(drivers); ++i) {
        if (drivers[i]->Probe()) {
            // driver found
            g_Driver = drivers[i];
        }
    }

    if (g_Driver == NULL) {
        printk("WARNING: No PIC found\n");
        return;
    }

    printk("Found %s\n", g_Driver->Name);
    g_Driver->Initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    // register ISR handlers for each of the 16 irq lines
    for (int i = 0;i < 16; ++i) {
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
    }

    // enable interrupts
    i686_EnableInts();
    g_Driver->Unmask(0);        // PIT
    g_Driver->Unmask(1);        // keyboard
    g_Driver->Unmask(2);        // slave PIC, no handler needed
    g_Driver->Unmask(8);        // RTC
    g_Driver->Unmask(12);       // mouse
    // g_Driver->Unmask(14);       // master ATA
    // g_Driver->Unmask(15);       // slave ATA
}
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler) {
    g_IRQHandlers[irq] = handler;
}