#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
represents a physical or emulated disk device
*/
typedef struct {
    uint8_t id;         // drive number or BIOS disk ID
    uint16_t cylinders; // number of cylinders on the disk
    uint16_t sectors;   // number of sectors per track
    uint16_t heads;     // number of heads (surfaces)
} DISK;

bool DISK_initialize(DISK* disk, uint8_t driveNum);
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* lowerDataOut);
