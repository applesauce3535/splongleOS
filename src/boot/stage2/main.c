#include "stdint.h"
#include "stdio.h"
#include "fat.h"
#include "disk.h"

#pragma aux _cstart "*"

void _cdecl _cstart(uint16_t bootDrive) {
    DISK disk;
    if (!DISK_initialize(&disk, bootDrive)) {
        printf("disk init error\r\n");  // disk initialization fail
        goto end;
    }

    if (!FAT_Initialize(&disk)) {
        printf("FAT init error\r\n");  // FAT initialization fail
        goto end;
    }

    // browse files in root
    FAT_File far* fd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    int i = 0;
    while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 5) {
        printf("   ");
        for (int i = 0; i < 11; ++i) {
            putc(entry.Name[i]);
        }
        printf("\r\n");
    }
    FAT_Close(fd);

    // read test.txt
    char buffer[100];
    fd = FAT_Open(&disk, "bitch/test.txt");
    uint32_t read;
    while ((read = FAT_Read(&disk, fd, sizeof(buffer), buffer))) {
        for (uint32_t i = 0; i < read; ++i) {
            putc(buffer[i]);
        }
    }
    FAT_Close(fd);

end:
    for(;;);
}
