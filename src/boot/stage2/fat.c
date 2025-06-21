#include "fat.h"
#include "stdio.h"
#include "memdefs.h"
#include "utility.h"

#pragma pack(push, 1)

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIRECTORY_HANDLE -1

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
    
} FAT_BootSector; 

#pragma pack(pop)

typedef struct {
    FAT_File Public;        // contains data returned to the user
    bool Opened;            // tracks whether a slot in OpenedFiles is open
    uint32_t FirstCluster;
    uint32_t CurrentCluser;
    uint32_t CurrentSectorInCluser;
    uint8_t Buffer[SECTOR_SIZE];    // size of a sector is the minimum we can read from a disk at once
} FAT_FileData;             // contains all info needed to read from file

typedef struct {
    union {start of FAT data region
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;
    FAT_FileData OpenedFiles[MAX_FILE_HANDLES]; // an array of opened files, max size can be decreased for more memory or increased if more files must be opened

} FAT_Data;

// nothing else should be messing with the FAT driver so these should be static
static FAT_Data far* g_Data;
static uint8_t far* g_FAT = NULL;
static uint32_t g_DataSectionLba;

bool FAT_ReadBootSector(DISK* disk) {
    /*
    Reads the boot sector from the disk image into byte array.
    */

    return DISK_ReadSectors(disk, 0, 1, &g_Data->BS,BootSectorBytes);
}

bool FAT_ReadFAT (DISK* disk) {
    return DISK_readSectors(disk, g_Data->BS.BootSector.ReservedSectors, g_Data->BS.BootSector.SectorsPerFat, g_FAT);
}

bool FAT_ReadRootDirectory(DISK* disk) {
    // beginning of root directory should be after reserved sectors and FAT tables
    uint32_t lba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
    // get the size of the root directory in bytes
    uint32_t size = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;
    // the number of sectors to read is the size of the root directory / bytes per sector
    uint32_t sectors = (size + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
    // must round up to ensure all sectors are read
    if(size % g_Data->BS.BootSector.BytesPerSector > 0) {
        sectors++;
    }
    g_RootDirectoryEnd = lba + sectors; // this is where the data section begins
    return DISK_ReadSectors(disk, lba, sectors, g_RootDirectory);
}

bool FAT_Initialize(DISK* disk) {
    // sets up FAT driver at the expected mem address
    g_Data = (FAT_Data far*)MEMORY_FAT_ADDR;
    // reads boot sector
    if (!FAT_ReadBootSector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }
    // read FAT
    g_FAT = (uint8_t far*)(g_Data + sizeof(FAT_Data));
    uint32_t FAT_size = g_Data->BS.BootSector.BytesPerSector * g_Data->BS.BootSector.SectorsPerFat;
    // ensure we aren't using more memory than allowed
    if (sizeof(FAT_Data) + FAT_size >= MEMORY_FAT_SIZE) {
        printf("FAT: not enough memory to read FAT. Require %lu, have %u.\r\n", sizeof(FAT_Data) + FAT_size, MEMORY_FAT_SIZE);
        return false;
    }
    if (!FAT_ReadFAT(disk)) {
        printf("FAT: read FAT failed.\r\n");
        return false;
    }
    // read root dir
    uint32_t rootDirLba = g_Data->BS.BootSector.ReservedSectors + g_Data->BS.BootSector.SectorsPerFat * g_Data->BS.BootSector.FatCount;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * g_Data->BS.BootSector.DirEntryCount;    // root size should be the size of a dir * how many dirs there are


    // open root dir
    g_Data->RootDirectory.Opened = true; 
    g_Data->RootDirectory.Public.Handle = ROOT_DIRECTORY_HANDLE;
    g_Data->RootDirectory.Public.IsDirectory = true;
    g_Data->RootDirectory.Public.Position = 0;
    g_Data->RootDirectory.Public.Size = rootDirSize;
    g_Data->RootDirectory.FirstCluster = 
    g_Data->RootDirectory.CurrentCluser = 0;
    g_Data->RootDirectory.CurrentSectorInCluser = 0;

    if (!DISK_ReadSectors(disk, rootDirLba, 1, g_Data->RootDirectory.Buffer)) {
        printf("FAT: read root directory failed\r\n");
        return false;
    }

    // calculate data section
    uint32_t RootDirSectors = (rootDirSize + g_Data->BS.BootSector.BytesPerSector - 1) / g_Data->BS.BootSector.BytesPerSector;
    g_DataSectionLba = rootDirLba + RootDirSectors;

    // reset opened files
    for (int i = 0; i < MAX_FILE_HANDLES; ++i) {
        g_Data->OpenedFiles[i].Opened = false;
    }
}

FAT_File far* FAT_Open(DISK* disk, const char* path) {

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
    // error message if file could not be read
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