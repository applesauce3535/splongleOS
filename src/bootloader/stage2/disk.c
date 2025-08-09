#include "disk.h"
#include "x86.h"

/*
initializes a DISK structure by querying drive parameters via BIOS

disk - pointer to the disk structure to initialize
driveNum - BIOS drive number
returns true if initialization succeeded, false otherwise
*/
bool DISK_initialize(DISK* disk, uint8_t driveNum) {
    uint8_t driveType;
    uint16_t cylinders, sectors, heads;
    if(!x86_Disk_getDriveParams(disk->id, &driveType, &cylinders, &sectors, &heads)) {
        return false;
    }
    disk->id = driveNum;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;
    return true;
}

/*
converts a logical block address to cylinder-head-sector format

disk - pointer to the disk structure containing geometry info
lba - lba to convert
cylindersOut - pointer for the resulting cylinder number
sectorsOut - pointer for the resulting sector number
headsOut - pointer for the resulting head number
*/
void DISK_LBA2CHS(DISK* disk, uint32_t lba, uint16_t* cylindersOut, uint16_t* sectorsOut, uint16_t* headsOut) {
    // sectors = (LBA % sectors per track + 1)
    *sectorsOut = lba % disk->sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylindersOut = (lba / disk->sectors) / disk->heads;

    // head = (LBA / sectors per track) % heads
    *headsOut = (lba / disk->sectors) % disk->heads;
}

/*
reads one or more sectors from the disk using CHS addressing

disk - pointer to the disk structure
lba - lba of the firs sector to read
sectors - number of sectors to read
dataOut - pointer to destination buffer for the sector data
return true if the read succeeded, false after 3 retry attempts
*/
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, void* lowerDataOut) {
    uint16_t cylinder, sector, head;
    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for(int i = 0; i < 3; ++i) {
        if(x86_Disk_Read(disk->id, cylinder, sector, head, sectors, lowerDataOut)) {
            return true;
        }
        x86_Disk_Reset(disk->id);
    }
    return false;
}
