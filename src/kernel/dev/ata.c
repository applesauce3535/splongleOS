#include "include/ata.h"

static bool int14_on = false;
static bool int15_on = false;

static inline void ata_400ns_delay(uint16_t control_port) {
    i686_inb(control_port);
    i686_inb(control_port);
    i686_inb(control_port);
    i686_inb(control_port);
}


// primary bus uses this
void int14(Registers* regs) {
    printk("This is interrupt 14\n");
}

// secondary bus uses this
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

void Identify(AdvancedTechnologyAttachment ata) {
    // clears HOB (high order byte) bit, if 1 enables access to large drives beyond 28-bit addressing
    i686_outb(ata.control_port, 0);
    // determine if we are communicating with a master or slave device
    uint8_t devsel = 0xA0 | (ata.master ? 0x00 : 0x10);
    i686_outb(ata.device_port, devsel);
    ata_400ns_delay(ata.control_port);
    uint8_t status = i686_inb(ata.command_port);
    if (status == 0xFF) {
        printk("No device on bus\n");
        return;
    }
    // printk("First if statement passed\n");

    i686_outb(ata.sector_count_port, 0);
    i686_outb(ata.lba_low_port, 0);
    i686_outb(ata.lba_mid_port, 0);
    i686_outb(ata.lba_high_port, 0);

    // 0xEC is the command for identifying a device
    i686_outb(ata.command_port, 0xEC);

    while (i686_inb(ata.command_port) & 0x80);  // BSY status

    status = i686_inb(ata.command_port);
    if (status & 0x01) {
        printk("IDENTIFY error\n");
        return;
    }

    while (!(i686_inb(ata.command_port) & 0x08)); // DRQ status

    // once reaching this point, data is ready to be read
    for (uint16_t i = 0; i < 256; ++i) {    // reading 2-byte ints from data port . 512 bytes per sector / 2 = 256
        uint16_t data = i686_inw(ata.data_port);
        char* foo = "  \0";
        foo[1] = (data >> 8) & 0x00FF;
        foo[0] = data & 0x00FF;
        printk(foo);
    }
    printk("ATA device identified\n");
}

void Read28(uint32_t sector) {

}

void Write28(uint32_t sector, uint8_t* data, uint32_t count) {

}