#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver.h"
#include "irq.h"
#include "stdio.h"
#include "asm_wrappers.h"

typedef struct {
    uint16_t data_port;         // port to send data to write and pull data to read
    uint8_t error_port;         // reading error messages
    uint8_t sector_count_port;  // how many sectors we want to read
    uint8_t lba_low_port;       // bits 0-7 of LBA to read
    uint8_t lba_mid_port;       // bits 8-15 of LBA to read
    uint8_t lba_high_port;      // bits 15-21 of LBA to read
    uint8_t device_port;        // communicate with master or slave drive, and transfer part of LBA on this port too
    uint8_t command_port;       // port for transfering instructions (read/write)
    uint8_t control_port;       // used for controlling device + reading device status messages
    bool master;
    bool initialized;
    uint16_t bytes_per_sector;
} AdvancedTechnologyAttachment;

AdvancedTechnologyAttachment ATA_Init(uint16_t port_base, bool master);

// check that drive exists and see what kind it is
void Identify();
void Read28(uint32_t sector);
void Write28(uint32_t sector, uint8_t* data, uint32_t count);
void Flush();