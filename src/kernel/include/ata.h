#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver.h"
#include "irq.h"
#include "stdio.h"
#include "asm_wrappers.h"

// primary port defines
#define ATA_PRIMARY_MASTER_PORT         0x1F0
#define ATA_PRIMARY_SLAVE_PORT          0x170
#define ATA_PRIMARY_MASTER_CTRL_PORT    0x3F6
#define ATA_PRIMARY_SLAVE_CTRL_PORT     0x376

// secondary port defines
#define ATA_SECONDARY_MASTER_PORT         0x1E8
#define ATA_SECONDARY_SLAVE_PORT          0x168
#define ATA_SECONDARY_MASTER_CTRL_PORT    0x3E6
#define ATA_SECONDARY_SLAVE_CTRL_PORT     0x366

#define ATA_MAX_ADDRESSABLE_SPACE         0x0FFFFFFF

// ATA commands
typedef enum {
    CACHE_FLUSH     = 0xE7,
    IDENTIFY        = 0xEC,
    READ            = 0x20,
    WRITE           = 0x30
} ATA_COMMANDS;

typedef struct {
    uint16_t data_port;             // port to send data to write and pull data to read
    uint16_t error_port;            // reading error messages
    uint16_t sector_count_port;     // how many sectors we want to read
    uint16_t lba_low_port;          // bits 0-7 of LBA to read
    uint16_t lba_mid_port;          // bits 8-15 of LBA to read
    uint16_t lba_high_port;         // bits 15-21 of LBA to read
    uint16_t device_port;           // communicate with master or slave drive, and transfer part of LBA on this port too
    uint16_t command_port;          // port for transfering instructions (read/write)
    uint16_t control_port;          // used for controlling device + reading device status messages
    bool master;
    bool initialized;
    uint16_t bytes_per_sector;
} AdvancedTechnologyAttachment;

AdvancedTechnologyAttachment ATA_Init(uint16_t port_base);

// check that drive exists and see what kind it is
void ATA_Identify(AdvancedTechnologyAttachment* ata);
void ATA_Read28(uint32_t sector, uint32_t count, AdvancedTechnologyAttachment* ata);
void ATA_Write28(uint32_t sector, uint8_t* data, uint32_t count, AdvancedTechnologyAttachment* ata);
void ATA_Reset(AdvancedTechnologyAttachment* ata);