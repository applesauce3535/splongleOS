#pragma once

#include "disk.h"
#include "stdint.h"

/*
represents a single entry in a FAT directory table
holds metadat about a file or subdirectory
*/
typedef struct {
    uint8_t Name[11];           // 8.3 filename (padded with spaces, no null terminator)
    uint8_t Attributes;         // file attribute flags (see FAT_Attributes enum)
    uint8_t _Reserved;          // reserved byte
    uint8_t CreatedTimeTenths;  // creation time (tenths of a second)
    uint16_t CreatedTime;       // time file was created (HH:MM:SS)
    uint16_t CreatedDate;       // date file was created (YYYY-MM-DD)
    uint16_t AccessedDate;      // last access date (YYYY-MM-DD)
    uint16_t FirstClusterHigh;  // high word of the starting cluster (FAT32 only)
    uint16_t ModifiedTime;      // time of last modification
    uint16_t ModifiedDate;      // date of last modification
    uint16_t FirstClusterLow;   // low word of the starting cluster
    uint32_t Size;              // size of the file in bytes
} __attribute__((packed)) FAT_DirectoryEntry;   // packed attribute eliminates padding between struct members


/*
represents an open file or directory handle in the FAT file system
used to track the state and access position of the file during I/O
*/
typedef struct {
    int Handle;         // unique ID for the open file handle
    bool IsDirectory;   // true if the handle represents a directory
    uint32_t Position;  // current read/write position in the file
    uint32_t Size;      // size of the file in bytes
} FAT_File;

/*
flags used in FAT_DirectoryEntry.Attributes to indicate file type and status
*/
enum FAT_Attributes {
    FAT_ATTRIBUTE_READ_ONLY         = 0x01, // file is read only
    FAT_ATTRIBUTE_HIDDEN            = 0x02, // file is hidden
    FAT_ATTRIBUTE_SYSTEM            = 0x04, // file is a system file
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08, // entry is a volume label
    FAT_ATTRIBUTE_DIRECTORY         = 0x10, // entry is a directory
    FAT_ATTRIBUTE_ARCHIVE           = 0x20, // file is marked for backup/archive
    // combination of flags indicating a long file name (LFN) entry
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool FAT_ReadBootSector(DISK* disk);
bool FAT_ReadFAT (DISK* disk);
bool FAT_Initialize(DISK* disk);
uint32_t FAT_ClusterToLba(uint32_t cluster);
FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry);
uint32_t FAT_NextCluster(uint32_t currentCluster);
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut);
bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File* file);
bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut);
FAT_File* FAT_Open(DISK* disk, const char* path);
