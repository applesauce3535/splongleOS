#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"

typedef struct {
    // FAT info
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectors;

    // boot info
    uint8_t DriveNumber;
    uint8_t _Reserved;        // reserved byte
    uint8_t Signature;
    uint32_t VolumeID;        // serial number
    uint8_t VolumeLabel[11];  // 11 bytes, padded with spaces
    uint8_t SystemID[8];      // 8 bytes, padded with spaces
    
} BootSector;

BootSector g_BootSector;

bool readBootSector(FILE* disk) {
    /*
    Reads the boot sector from the disk image into g_BootSector.

    - Uses fread to read raw bytes from the file.
    - Reads 1 block of size sizeof(BootSector) from the disk.
    - &g_BootSector is the destination buffer.
    - 'disk' is the FILE* pointing to the opened disk image.
    - Returns true if 1 block was successfully read, false otherwise.
    */

    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");
    return 0;
}