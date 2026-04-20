#include "include/ata.h"

static bool int14_on = false;
static bool int15_on = false;
static uint8_t g_last_device = 0x0;

static inline void ATA_420ns_delay(uint16_t control_port) {
    for (uint8_t i = 0; i < 15; ++i) i686_inb(control_port);
}

static inline void ATA_Dev_select(AdvancedTechnologyAttachment* ata, uint8_t device) {
    i686_outb(ata->device_port, device);
    ATA_420ns_delay(ata->control_port);
    g_last_device = device;
}


// primary bus uses this
void int14(Registers* regs) {
    printk("This is interrupt 14\n");
}

// secondary bus uses this
void int15(Registers* regs) {
    printk("This is interrupt 15\n");
}

AdvancedTechnologyAttachment ATA_Init(uint16_t port_base) {
    AdvancedTechnologyAttachment dev = {
        .bytes_per_sector = 512,
        // FIX THIS BECAUSE IT DOESNT WORK FOR SECONDARY ATAS
        .master = (port_base == ATA_PRIMARY_MASTER_PORT ? true : false),
        .data_port = port_base,
        .error_port = port_base + 1,
        .sector_count_port = port_base + 2,
        .lba_low_port = port_base + 3,
        .lba_mid_port = port_base + 4,
        .lba_high_port = port_base + 5,
        .device_port = port_base + 6,
        .command_port = port_base + 7,
        // FIX THIS LATER BECAUSE IT DOESNT WORK FOR SECONDARY ATAS
        .control_port = (port_base == ATA_PRIMARY_MASTER_PORT ? 0x3F6 : 0x376),
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

void ATA_Identify(AdvancedTechnologyAttachment* ata) {
    // select master/slave
    // send 0xA0 to select the master to the device port, send 0xB0 to select the slave
    // to device port
    // if we need to select a different device, we need to wait for the device to respond
    uint8_t device_to_select = ata->master ? 0xA0 : 0xB0;
    if (g_last_device != device_to_select) {
        ATA_Dev_select(ata, device_to_select);
    }

    // clears HOB (high order byte) bit, if 1 enables access to 48-bit addressing
    i686_outb(ata->control_port, 0);

    // check the status of selected device, 0xFF is a "floating device"
    uint8_t status = i686_inb(ata->command_port);
    if (status == 0xFF) {
        printk("No device on bus: 0x%x\n", status);
        return;
    }

    // next set sector count and LBA ports all to 0
    i686_outb(ata->sector_count_port, 0);
    i686_outb(ata->lba_low_port, 0);
    i686_outb(ata->lba_mid_port, 0);
    i686_outb(ata->lba_high_port, 0);

    // send IDENTIFY command to command port
    i686_outb(ata->command_port, IDENTIFY);

    // read status again, if status is 0 then the drive doesn't exist
    status = i686_inb(ata->command_port);
    if (status == 0) {
        printk("Drive does not exist\n");
        return;
    }

    // check if ATA or ATAPI (CD ROM)
    // we have to do this first because some ATAPI devices do not set BSY after IDENTIFY
    uint8_t lba_mid = i686_inb(ata->lba_mid_port);
    uint8_t lba_high = i686_inb(ata->lba_high_port);
    // ATAPI devices have a non-0 signature
    if (lba_mid != 0 || lba_high != 0) {
        printk("ATAPI device detected (probably CD-ROM)\n");
        return;
    }
    // now poll status until bit 7 (BSY bit, 0x80) clears
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set
    while (!(status & 0x08)) {
        if (status & 0x01) {
            printk("Error during ATA identify\n");
            return;
        }
        status = i686_inb(ata->command_port);
    }

    // by this point data is ready to be read
    // skip the first 27 words
    for (int i = 0; i < 27; ++i) {
        i686_inw(ata->data_port);
    }
    // read words 27-46
    char model[41];
    for (int i = 0; i < 20; ++i) {
        // extract the HDD model
        uint16_t data = i686_inw(ata->data_port);
        model[i*2] = (data >> 8) & 0xFF;
        model[i*2+1] = data & 0xFF;
    }
    // trim padded spaces
    for (int i = 39; i >= 0; i--) {
        if (model[i] == ' ') model[i] = '\0';
        else break;
    }
    // very hacked way to print the model
    model[40] = '\0';
    printk("Model: ");
    for (int i = 0; i < 40; ++i) {
        printk("%c", model[i]);
    }
    printk("\n");
}

/*
    Caller must make sure the passed buffer is large enough to contain all requested data, which would be count
*/
void ATA_Read28(uint32_t sector, uint32_t count, AdvancedTechnologyAttachment* ata) {
    // you can't read to a sector larger than what you can address with 28 bits, so make sure first 4 are 0
    if (sector > ATA_MAX_ADDRESSABLE_SPACE) {
        printk("Cannot read sector that's > 28 bits: sector = 0x%x\n", sector);
        return;
    }

    // can't read more than the bytes per sector in one read
    if (count > ata->bytes_per_sector) {
        printk("Tried reading too much data for a single sector: count %d\n", count);
        return;
    }

    // select device
    uint8_t device_to_select = (ata->master ? 0xE0 : 0xF0) | ((sector >> 24) & 0x0F);
    if (g_last_device != device_to_select) {
        ATA_Dev_select(ata, device_to_select);
    }

    // send the sectorcount to the sector count port
    i686_outb(ata->sector_count_port, 1);

    // send low, mid, and high bits of LBA to their respective ports
    i686_outb(ata->lba_low_port, (uint8_t)sector);
    i686_outb(ata->lba_mid_port, (uint8_t)(sector >> 8));
    i686_outb(ata->lba_high_port, (uint8_t)(sector >> 16));

    // send the READ SECTORS command to the command port
    i686_outb(ata->command_port, READ);

    // do the same polling as from ATA_Identify
    uint8_t status = i686_inb(ata->command_port);
    // poll for BSY
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set, but ignore first 4 ERR polls
    uint8_t polls_ignored = 4;
    while (!(status & 0x08)) {
        if (status & 0x01 && polls_ignored < 0) {
            printk("Error during ATA read\n");
            return;
        }
        status = i686_inb(ata->command_port);
        --polls_ignored;
    }
    
    // transfer 256 16-bit values, one 16-bit amount at a time
    for (uint16_t i = 0; i < ata->bytes_per_sector; i += 2) {
        uint16_t rdata = i686_inw(ata->data_port);
        // this chunk gets skipped once we've read count bytes, but the above will complete until
        // a full sector is read
        if (i < count) {
            printk("%c", rdata & 0xFF);
            // odd number of count protection
            if (i + 1 < count) {
                printk("%c", (rdata >> 8) & 0xFF);
            }
        }
    }
    // give drive a delay to reset DRQ and set BSY again
    ATA_420ns_delay(ata->control_port);

    status = i686_inb(ata->command_port);
    
    // poll a final time
    // poll for BSY
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set, but ignore first 4 ERR polls
    while (!(status & 0x08)) {
        if (status & 0x01) {
            printk("Error during ATA read\n");
            return;
        }
        status = i686_inb(ata->command_port);
    }
}

void ATA_Write28(uint32_t sector, uint8_t* data, uint32_t count, AdvancedTechnologyAttachment* ata) {
    // you can't write to a sector larger than what you can address with 28 bits
    if (sector > ATA_MAX_ADDRESSABLE_SPACE) {
        printk("Cannot address sector > 28 bits: sector 0x%x\n", sector);
        return;
    }
    if (count > ata->bytes_per_sector) {
        printk("Tried writing too much data for a single sector: count %d\n", count);
        return;
    }

    // select device
    uint8_t device_to_select = (ata->master ? 0xE0 : 0xF0) | ((sector >> 24) & 0x0F);
    if (g_last_device != device_to_select) {
        ATA_Dev_select(ata, device_to_select);
    }

    // send 0 to error port and sector count to sector count port
    i686_outb(ata->error_port, 0);
    i686_outb(ata->sector_count_port, 1);

    // send low, mid, and high LBA bits to respective ports
    i686_outb(ata->lba_low_port, (uint8_t)sector);
    i686_outb(ata->lba_mid_port, (uint8_t)(sector >> 8));
    i686_outb(ata->lba_high_port, (uint8_t)(sector >> 16));

    // send write command to command port
    i686_outb(ata->command_port, WRITE);
    ATA_420ns_delay(ata->control_port);

    // poll as usual
    uint8_t status = i686_inb(ata->command_port);
    // poll for BSY
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set, but ignore first 4 ERR polls
    uint8_t polls_ignored = 4;
    while (!(status & 0x08)) {
        if (status & 0x01 && polls_ignored < 0) {
            printk("Error during ATA read\n");
            return;
        }
        status = i686_inb(ata->command_port);
        --polls_ignored;
    }

    // device is now ready to write the data
    // the device always wants a full sector to be written, even if it's nothing data, otherwise it will throw an error
    for (uint16_t i = 0; i < ata->bytes_per_sector; i += 2) {
        uint16_t wdata = 0;

        if (i < count) {
            wdata = data[i];
        /*
        ATA sends write data in 16-bit transfers. x86 uses little endian, so we want the first
        byte to be low and the next byte to be high. the line before this would make wdata look like 0x0034.
        if the next data byte is 0x12, first convert it to 16 bits so now it's 0x0012, shift that 8 bits so it's
        0x1200, and OR it with wdata so it becomes 0x1234. now it's ready to send, and because of little endian, ATA
        will read it like: first byte 0x34, second byte 0x12. the conditional is for odd numbers of data transfers.
        */
            if (i + 1 < count) {
                wdata |= ((uint16_t)data[i + 1]) << 8;
            }
        }
        i686_outw(ata->data_port, wdata);
        // after each out OUT there must be a tiny delay
        __asm__ __volatile__("jmp .+2");    // this will simply go to the next instruction
                                            // but add a little delay
    }
    
    ATA_420ns_delay(ata->control_port);

    // poll before flushing cache
    status = i686_inb(ata->command_port);
    // poll for BSY
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set, but ignore first 4 ERR polls
    polls_ignored = 4;
    while (!(status & 0x08)) {
        if (status & 0x01 && polls_ignored < 0) {
            printk("Error during ATA read\n");
            return;
        }
        status = i686_inb(ata->command_port);
        --polls_ignored;
    }

    // flush cache after each write completes
    i686_outb(ata->command_port, CACHE_FLUSH);

    // poll after flush as well 
    status = i686_inb(ata->command_port);
    // poll for BSY
    while (status & 0x80) status = i686_inb(ata->command_port);

    // next poll for DRQ (0x08) or ERR (0x01) bits being set, but ignore first 4 ERR polls
    polls_ignored = 4;
    while (!(status & 0x08)) {
        if (status & 0x01 && polls_ignored < 0) {
            printk("Error during ATA read\n");
            return;
        }
        status = i686_inb(ata->command_port);
        --polls_ignored;
    }
}