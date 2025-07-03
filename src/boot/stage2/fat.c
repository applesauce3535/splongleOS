#include "fat.h"
#include "stdio.h"
#include "memdefs.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"
#include <stddef.h>
#include "minmax.h"

#define SECTOR_SIZE 512             // The size of a sector in bytes. The smallest unit readable from disk.
#define MAX_PATH_SIZE 256           // Maximum path length supported (similar to Windows' 255-character limit).
#define MAX_FILE_HANDLES 10         // Maximum number of concurrently open files supported. Can be changed if necessary
#define ROOT_DIRECTORY_HANDLE -1    // Special handle used to refer to the root directory.

typedef struct {
    // Represents the 512-byte FAT12/16 boot sector.
    // Fields are packed and closely match FAT specifications.
    // FAT info
    uint8_t BootJumpInstruction[3]; // Bootstrap code (often 0xEB 0x?? 0x90)
    uint8_t OemIdentifier[8];       // OEM name/version (doesn't really matter)
    uint16_t BytesPerSector;        // Typically 512
    uint8_t SectorsPerCluster;      // Cluster size in sectors
    uint16_t ReservedSectors;       // Number of reserved sectors before the FATs
    uint8_t FatCount;               // Number of FAT copies (usually 2 in case one fails)
    uint16_t DirEntryCount;         // Number of root directory entries
    uint16_t TotalSectors;          // Number of sectors
    uint8_t MediaDescriptorType;    // Device this is all stored on (a floppy in my case)
    uint16_t SectorsPerFat;         // Sectors per FAT table
    uint16_t SectorsPerTrack;       // Disk geometry
    uint16_t Heads;                 // Disk geometry
    uint32_t HiddenSectors;         // Sectors before start of partition
    uint32_t LargeSectors;          // Doesn't matter for a 1.4MB floppy

    // boot info
    uint8_t DriveNumber;        // Physical drive number (what disk are we using)
    uint8_t _Reserved;          // Reserved byte (necessary, set to 0 but really could be anything)
    uint8_t Signature;          // Boot signature (0x29 means presence of VolumeID)
    uint32_t VolumeID;          // serial number
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemID[8];        // 8 bytes, padded with spaces, file system type (FAT12)
} __attribute__((packed)) FAT_BootSector; 


typedef struct {
    // Holds internal file state for an open file or directory.
    FAT_File Public;                    // contains data returned to the user (handle, position, size)
    bool Opened;                        // tracks whether a slot in OpenedFiles is open
    uint32_t FirstCluster;              // first cluster of the file
    uint32_t CurrentCluster;            // currently accessed cluster
    uint32_t CurrentSectorInCluster;    // offset in sectors within the current cluster
    uint8_t Buffer[SECTOR_SIZE];        // current sector buffer
} FAT_FileData;

typedef struct {
    // All state required by the FAT driver.
    union {
        FAT_BootSector BootSector;              // parsed boot sector structure
        uint8_t BootSectorBytes[SECTOR_SIZE];   // raw boot sector bytes
    } BS;

    FAT_FileData RootDirectory;                 // state of root
    FAT_FileData OpenedFiles[MAX_FILE_HANDLES]; // an array of opened files and available file slots
} FAT_Data;

// nothing else should be messing with the FAT driver so these should be static
static FAT_Data* g_Data;        // Pointer to FAT driver's memory space.
static uint8_t* g_FAT = NULL;   // Pointer to in-memory FAT table (loaded during initialization).
static uint32_t g_DataSectionLba;   // LBA offset where the data region (file contents) starts.

/*
reads the FAT boot sector from the given disk into memory

disk - pointer to the disk structure to read from
returns true if the boot sector was successfully read, false otherwise
*/
bool FAT_ReadBootSector(DISK* disk) {
    return DISK_ReadSectors(disk, 0, 1, &g_Data->BS.BootSectorBytes);
}

/*
reads the FAT table from disk into memory

disk - pointer to the disk structure to read from
returns true if the FAT table was successfully loaded, false otherwise
*/
bool FAT_ReadFAT (DISK* disk) {
    return DISK_ReadSectors(disk, g_Data->BS.BootSector.ReservedSectors, g_Data->BS.BootSector.SectorsPerFat, g_FAT);
}

/*
initializes the FAT driver by reading and parsing required disk structures

disk - pointer to the disk to initialize FAT from
returns true if initialization was successful, false otherwise
*/
bool FAT_Initialize(DISK* disk) {
    // sets up FAT driver at the expected mem address
    g_Data = (FAT_Data*)MEMORY_FAT_ADDR;
    // reads boot sector
    if (!FAT_ReadBootSector(disk)) {
        printf("FAT: read boot sector failed\r\n");
        return false;
    }
    // read FAT
    g_FAT = (uint8_t*)(g_Data + sizeof(FAT_Data));
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
    g_Data->RootDirectory.FirstCluster = rootDirLba;
    g_Data->RootDirectory.CurrentCluster = rootDirLba;
    g_Data->RootDirectory.CurrentSectorInCluster = 0;

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

    return true;
}

/*
converts a FAT cluster number to a logical block address (LBA)

cluster - cluster number (must be >= 2)
returns LBA corresponding to the passed cluster number
*/
uint32_t FAT_ClusterToLba(uint32_t cluster) {
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.BootSector.SectorsPerCluster;
}

/*
opens a file or directory represented by a directory entry

disk - pointer to the disk structure
entry - pointer to the FAT directory entry to open
returns a pointer to a FAT file structure for the opened file, or NULL if one isn't available
*/
FAT_File* FAT_OpenEntry(DISK* disk, FAT_DirectoryEntry* entry) {
    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; ++i) {
        if (!g_Data->OpenedFiles[i].Opened) {
            handle = i;
        }
    }

    // out of handles
    if (handle < 0) {
        printf("FAT: out of file handles\r\n");
        return false;
    }

    // setup vars
    FAT_FileData* fd = &g_Data->OpenedFiles[handle];
    fd->Public.Handle = handle;
    // find if entry is a directory
    fd->Public.IsDirectory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    // position is 0 to read from beginning of the file
    fd->Public.Position = 0;
    fd->Public.Size = entry->Size;
    // first cluster is split into 2 words so add them together
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    // current cluster same as first cluster
    fd->CurrentCluster = fd->FirstCluster;
    // current sector in cluster is 0
    fd->CurrentSectorInCluster = 0;

    // read first sector of file into buffer
    if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster), 1, fd->Buffer)) {
        printf("FAT: open entry read error\r\n");
        for (int i = 0; i < 11; ++i) {
            printf("%c", entry->Name[i]);
        }
        printf("\n");
        return false;
    }

    fd->Opened = true;
    return &fd->Public;
}

/*
retrieves next cluster in the chain for a given cluster number

currentCluster - the current cluster in the chain
returns the next cluster number, or 0xFFF if end of chain
*/
uint32_t FAT_NextCluster(uint32_t currentCluster) {
    // formula for getting next cluster index in the FAT
    uint32_t fatIndex = currentCluster * 3 / 2;
    // if the current cluster is even, take the bottom 12 bits
    if (currentCluster % 2 == 0) {
        // converts the uint8* from g_FAT into uint16* and then dereferences that
        return (*(uint16_t*)(g_FAT + fatIndex)) & 0x0FFF; // bit mask to remove upper bits
    }
    // if the current cluster is odd, take the top 12 bits
    else {
        return (*(uint16_t*)(g_FAT + fatIndex)) >> 4; // shift by 4 bits
    }
}

/*
reads data from an opened file into a buffer

disk - pointer to the disk structure
file - pointer to the FAT file to read from
byteCount - number of bytes to read
dataOut - output buffer to store read data
returns number of bytes successfully read
*/
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut) {
    // get file data
    FAT_FileData* fd = (file->Handle == ROOT_DIRECTORY_HANDLE) ? &g_Data->RootDirectory : &g_Data->OpenedFiles[file->Handle];
    uint8_t* u8DataOut = (uint8_t*)dataOut;
    // don't read past end of file
    if (!fd->Public.IsDirectory || (fd->Public.IsDirectory && fd->Public.Size != 0)) {
        byteCount = min(byteCount, fd->Public.Size - fd->Public.Position);
    }
    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.Position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.Position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.Position += take;
        byteCount -= take;

        // see if we need to read more data
        if (leftInBuffer == take) {
            // special case for root dir
            if (fd->Public.Handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->CurrentCluster;
                // read next sector
                if (!DISK_ReadSectors(disk, fd->CurrentCluster, 1, fd->Buffer)) {
                    printf("FAT: read 1 read error\r\n");
                    break;
                }
            }
            else {
                // calculate next cluster and sector to read
                if (++fd->CurrentSectorInCluster >= g_Data->BS.BootSector.SectorsPerCluster) {
                    fd->CurrentSectorInCluster = 0;
                    fd->CurrentCluster = FAT_NextCluster(fd->CurrentCluster);
                }

                // check that we are not reading past the end of the file
                if (fd->CurrentCluster >= 0xFF8) {
                    // mark end of file
                    fd->Public.Size = fd->Public.Position;
                    break;
                }

                // read next sector
                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer)) {
                    printf("FAT: read 2 read error\r\n");
                    break;
                }
            }
        }
    }
    return u8DataOut - (uint8_t*)dataOut;   // number of bytes we just read
}

/*
reads a directory entry from an open directory stream

disk - pointer to disk structure
file - pointer to the FAT file representing a directory
dirEntry - output pointer for the next directory entry read
returns true if a valid entry was read, false if end of directory
*/
bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry) {
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
}

/*
closes an open file and frees its slot

file - pointer to the FAT file to close
*/
void FAT_Close(FAT_File* file) {
    if (file->Handle == ROOT_DIRECTORY_HANDLE) {
        file->Position = 0;
        g_Data->RootDirectory.CurrentCluster = g_Data->RootDirectory.FirstCluster;
    }
    else {
        g_Data->OpenedFiles[file->Handle].Opened = false;
    }
}

/*
searches for a file or directory by name within a directory

disk - pointer to disk structure
file - pointer to the FAT file representing the parent directory
name - name of the file or directory to find
entryOut - output pointer to store the found directory entry
returns true if the entry was found, false otherwise
*/
bool FAT_FindFile(DISK* disk, FAT_File* file, const char* name, FAT_DirectoryEntry* entryOut) {
    char fatName[11];
    FAT_DirectoryEntry entry;
    // convert from name to FAT name
    memset(fatName, ' ', sizeof(fatName));  // first pad with spaces
    const char* ext = NULL;
    const char* ptr = name;

    while ((ptr = strchr(ptr, '.')) != NULL) {  // safely ensures we get the final '.' in a file name
        ext = ptr;      // update ext to the latest dot found
        ++ptr;          // move past the current dot for the next iteration
    }
    
    if (ext == NULL) {
        ext = name + 11;
    }

    for (int i = 0; i < 8 && name[i] && name + i < ext; ++i) {
        fatName[i] = toupper(name[i]);
    }

    if (ext != name + 11) {
        for (int i = 0; i < 3 && ext[i + 1]; ++i) {
            fatName[i + 8] = toupper(ext[i + 1]);
        }
    }

    while(FAT_ReadEntry(disk, file, &entry)) {
        // check if the current directory entry matches what is stored in FAT name
        if (memcmp(fatName, entry.Name, 11) == 0) {
            *entryOut = entry;
            return true;
        }
    }
    return false;
}

/*
opens a file or directory by its absolute path

disk - pointer to the disk structure
path - null-terminated string containing the path
returns pointer to a FAT file structure for the opened file, or NULL if not found
*/
FAT_File* FAT_Open(DISK* disk, const char* path) {
    // by default Windows limits paths to 255, so I'll do the same
    char name[MAX_PATH_SIZE];
    
    // ignore leading slash
    if (path[0] == '/') {
        path++;
    }

    FAT_File* current = &g_Data->RootDirectory.Public;

    while (*path) {
        // extract next file name from path
        bool isLast = false;
        const char* delim = strchr(path, '/');
        if (delim != NULL) {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        }
        else {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            isLast = true;
        }
        // find dir entry in current dir
        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry)) {
            FAT_Close(current);
            // check if dir
            if (!isLast && entry.Attributes & FAT_ATTRIBUTE_DIRECTORY == 0) {
                printf("FAT: %s is not a directory\r\n", name);
                return NULL;
            }

            // open new entry
            current = FAT_OpenEntry(disk, &entry);
        }
        else {
            FAT_Close(current);
            printf("FAT: %s not found\r\n", name);
            return NULL;
        }
    }
    return current;
}
