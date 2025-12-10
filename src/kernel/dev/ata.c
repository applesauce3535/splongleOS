#include "include/ata.h"

static bool int14_on = false;
static bool int15_on = false;

void int14(Registers* regs) {
    printk("This is interrupt 14\n");
}

void int15(Registers* regs) {
    printk("This is interrupt 15\n");
}

AdvancedTechnologyAttachment ATA_Init(uint16_t port_base, bool master) {
    AdvancedTechnologyAttachment dev = {
        .bytes_per_sector = 512,
        .master = master,
        .data_port = port_base,
        .error_port = port_base + 1,
        .sector_count_port = port_base + 2,
        .lba_low_port = port_base + 3,
        .lba_mid_port = port_base + 4,
        .lba_high_port = port_base + 5,
        .device_port = port_base + 6,
        .command_port = port_base + 7,
        .control_port = port_base + 0x206,   // OF COURSE! HOW OBVIOUS!
        .initialized = false
    };

    // only do this on the first call to ATA_Init
    if (!int14_on && !int15_on) {
        i686_IRQ_RegisterHandler(14, &int14);
        int14_on = true;
        i686_IRQ_RegisterHandler(15, &int15);
        int15_on = true;
    }
    dev.initialized = true;
    return dev;
}

void Identify() {

}

void Read28(uint32_t sector) {

}

void Write28(uint32_t sector, uint8_t* data, uint32_t count) {

}