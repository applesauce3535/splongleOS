#include "pic.h"
#include "io.h"

#define PIC1_COMMAND_PORT   0x20
#define PIC1_DATA_PORT      0x21
#define PIC2_COMMAND_PORT   0xA0
#define PIC2_DATA_PORT      0xA1

/*
    Initialization Control Word 1
    -----------------------------
    0   ICw4    if set, the PIC expects to receive ICW4 during initialization
    1   SGNL    if set, only 1 PIC in the system, if unset, PIC cascaded with slave PIC
                and ICW3 must be sent to controller
    2   ADI     ignored on x86, set to 0
    3   LTIM    if set, operate in level triggered mode; if unset, operate in edge triggered mode
    4   INIT    set to 1 to initialize PIC
    5-7         ignored on x86, set to 0
*/

typedef enum {
    PIC_ICW1_ICW4       = 0x01,
    PIC_ICW1_SINGLE     = 0x02,
    PIC_ICW1_ADI        = 0x04,
    PIC_ICW1_LEVEL      = 0x08,
    PIC_ICW1_INIT       = 0x10
} PIC_ICW1;

/*
    Initialization Control Word 4
    -----------------------------
    0   uPM     if set, PIC is in 80x86 mode; if unset, in MCS-80/86 mode
    1   AEOI    if set,on last interrupt acknowledge pulse, controller automatically performs end of interrupt operation
    2   M/S     only use if BUF is set; if set, selects buffer master; otherwise, selects buffer slave
    3   BUF     if set, controller operates in buffered mode
    4   SFNM    specially fully nested mode; used in systems with large number of cascaded controllers (irrelevant here)
    5-7         reserved, set to 0
*/

typedef enum {
    PIC_ICW4_8086           = 0x01,
    PIC_ICW4_AUTO_EOI       = 0x02,
    PIC_ICW4_BUFFER_MASTER  = 0x04,
    PIC_ICW4_BUFFER_SLAVE   = 0x00,
    PIC_ICW4_BUFFER         = 0x08,
    PIC_ICW4_SFNM           = 0x10
} PIC_ICW4;

typedef enum {
    PIC_CMD_EOI         = 0x20,
    PIC_CMD_READ_IRR    = 0x0A,
    PIC_CMD_READ_ISR    = 0x0B
} PIC_CMD;


void i686_PIC_Config(uint8_t offsetPIC1, uint8_t offsetPIC2) {
    // initialize control word 1
    // uses ICW4, edge triggered mode, cascaded, and passes PIC init bit
    i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
    i686_io_wait();
    i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
    i686_io_wait();

    // initialize control word 2: the offsets
    i686_outb(PIC1_DATA_PORT, offsetPIC1);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, offsetPIC2);
    i686_io_wait();

    // initialize control word 3
    i686_outb(PIC1_DATA_PORT, 0x04);        // tell PIC1 it has a slave at IRQ2 (0000 0100)
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, 0x02);        // tell PIC2 its cascade identity (0000 0010)
    i686_io_wait();

    // initialize control word 4
    i686_outb(PIC1_DATA_PORT, PIC_ICW4_8086 | PIC_ICW4_AUTO_EOI);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, PIC_ICW4_8086 | PIC_ICW4_AUTO_EOI);
    i686_io_wait();

    // clear data registers
    i686_outb(PIC1_DATA_PORT, 0);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, 0);
    i686_io_wait();
}

void i686_PIC_Mask(int irq) {
    if (irq < 8) {
        // read current mask of PIC
        uint8_t mask = i686_inb(PIC1_DATA_PORT);
        i686_outb(PIC1_DATA_PORT, mask | (1 << irq));
    }
    else {
        irq -= 8;
        // read current mask of PIC
        uint8_t mask = i686_inb(PIC2_DATA_PORT);
        i686_outb(PIC2_DATA_PORT, mask | (1 << irq));
    }
}

void i686_PIC_Unmask(int irq) {
    uint8_t mask;
    if (irq < 8) {
        // read current mask of PIC
        mask = i686_inb(PIC1_DATA_PORT);
        i686_outb(PIC1_DATA_PORT, mask & ~(1 << irq));
    }
    else {
        irq -= 8;
        // read current mask of PIC
        mask = i686_inb(PIC2_DATA_PORT);
        i686_outb(PIC2_DATA_PORT, mask & ~(1 << irq));
    }
}

void i686_PIC_SendEndOfInt(int irq) {
    // if interrupt is from PIC2, send EOI to both PICs
    if (irq >= 8) i686_outb(PIC2_COMMAND_PORT, PIC_CMD_EOI);
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_EOI);
}

// if using APIC, you can disable the legacy PIC with this function
void i686_PIC_Disable() {
    i686_outb(PIC1_DATA_PORT, 0xFF);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, 0xFF);
    i686_io_wait();
}

uint16_t i686_PIC_ReadIRQRequestRegister() {
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return i686_inb(PIC2_COMMAND_PORT) | (i686_inb(PIC1_COMMAND_PORT) << 8);
}

uint16_t i686_PIC_ReadInServiceRegister() {
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return i686_inb(PIC2_COMMAND_PORT) | (i686_inb(PIC1_COMMAND_PORT) << 8);
}