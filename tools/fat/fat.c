#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

// reminder: lba = logical block address

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
    
} __attribute__((packed)) BootSector; // ensure compiler doesn't pad boot sector structure

typedef struct {
    // all the metadata FAT keeps for directories
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;

BootSector g_BootSector;
uint8_t* g_FAT = NULL;
DirectoryEntry* g_RootDirectory = NULL;
uint32_t g_RootDirectoryEnd;

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

bool readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferOut) {
    /*
    parameters:
        - disk: file handle
        - lba: sector number
        - count: sectors to read
        - bufferOut: pointer to address where data will be stored
    */
    bool ok = true;
    // before performing actual read, seek to the right position in the file
    ok = ok && (fseek(disk, lba * g_BootSector.BytesPerSector, SEEK_SET) == 0); // fseek returns 0 on success
    // read number of sectors from that location
    ok = ok && (fread(bufferOut, g_BootSector.BytesPerSector, count, disk) == count);
    return ok; // if anything fails ok should turn false
}

bool readFAT (FILE* disk) {
    // reserve enough memory for FATs
    g_FAT = (uint8_t*) malloc(g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector);
    // read the FAT
    return readSectors(disk, g_BootSector.ReservedSectors, g_BootSector.SectorsPerFat, g_FAT);
}

bool readRootDirectory(FILE* disk) {
    // beginning of root directory should be after reserved sectors and FAT tables
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.SectorsPerFat * g_BootSector.FatCount;
    // get the size of the root directory in bytes
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    // the number of sectors to read is the size of the root directory / bytes per sector
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    // must round up to ensure all sectors are read
    if(size % g_BootSector.BytesPerSector > 0) {
        sectors++;
    }
    g_RootDirectoryEnd = lba + sectors; // this is where the data section begins
    // root directory allocation
    g_RootDirectory = (DirectoryEntry*) malloc(sectors * g_BootSector.BytesPerSector);
    return readSectors(disk, lba, sectors, g_RootDirectory);
}

DirectoryEntry* findFile(const char* name) {
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; ++i) {
        if(memcmp(name, g_RootDirectory[i].Name, 11) == 0) {
            return &g_RootDirectory[i];
        }
    }
    return NULL;
}

bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer) {
    bool ok = true;
    uint16_t currentCluster = fileEntry->FirstClusterLow;
    do {
        // formula for converting from cluster to a sector
        uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2) * g_BootSector.SectorsPerCluster;
        // read 1 cluster using readSectors()
        ok = ok && readSectors(disk, lba, g_BootSector.SectorsPerCluster, outputBuffer);
        // advance output buffer position, it's just how many bytes it's moving forward
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector;
        
        // formula for getting next cluster index in the FAT
        uint32_t fatIndex = currentCluster * 3 / 2;
        // if the current cluster is even, take the bottom 12 bits
        if(currentCluster % 2 == 0) {
            // converts the uint8* from g_FAT into uint16* and then dereferences that
            currentCluster = (*(uint16_t*)(g_FAT + fatIndex)) & 0x0FFF; // bit mask to remove upper bits
        }
        // if the current cluster is odd, take the top 12 bits
        else {
            currentCluster = (*(uint16_t*)(g_FAT + fatIndex)) >> 4; // shift by 4 bits
        }
    } while(ok && currentCluster < 0x0FF8);
    return ok;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");

    // error message if image could not be opened
    if (!disk) {
        fprintf(stderr, "Cannot open disk image %s\n", argv[1]);
        return -1;
    }

    // error message if bootloader sector could not be read 
    if(!readBootSector(disk)) {
        fprintf(stderr, "Could not read boot sector\n");
        return -2;
    }

    // error message if FATs could not be read
    if(!readFAT(disk)) {
        fprintf(stderr, "Could not read FATs from disk\n");
        free(g_FAT); // free memory allocated for FAT
        free(g_RootDirectory);
        return -3;
    }

    // error message if root directory could not be read
    if(!readRootDirectory(disk)) {
        fprintf(stderr, "Could not read root directory\n");
        free(g_FAT);
        free(g_RootDirectory);
        return -4;
    }

    DirectoryEntry* fileEntry = findFile(argv[2]);
    // error message if file directory could not be found
    if(!fileEntry) {
        fprintf(stderr, "Could not find file %s on disk\n", argv[2]);
        free(g_FAT);
        free(g_RootDirectory);
        return -5;
    }

    // extra sector to avoid overwriting anything or segment faulting
    uint8_t* buffer = (uint8_t*) malloc(fileEntry->Size + g_BootSector.BytesPerSector);
    if(!readFile(fileEntry, disk, buffer)) {
        fprintf(stderr, "Could not read file %s on disk\n", argv[2]);
        free(g_FAT);
        free(g_RootDirectory);
        free(buffer);
        return -6;
    }

    for (size_t i = 0; i < fileEntry->Size; ++i) {
        if (isprint(buffer[i])) {
            fputc(buffer[i], stdout);
        }
        else {
            printf("<%02x>", buffer[i]);
        }
    }
    printf("\n");

    free(g_RootDirectory);
    free(g_FAT);
    free(buffer);
    return 0;
}