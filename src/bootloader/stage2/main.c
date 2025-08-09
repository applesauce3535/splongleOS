#include <stdint.h>
#include "stdio.h"
#include "x86.h"
#include "fat.h"
#include "disk.h"
#include "memdefs.h"
#include "memory.h"

uint8_t* kernelLoadBuffer = (uint8_t*)MEMORY_LOAD_KERNEL;
uint8_t* kernel = (uint8_t*)MEMORY_KERNEL_ADDR;

typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint16_t bootDrive) {
    clrscr();
    DISK disk;
    if (!DISK_initialize(&disk, bootDrive)) {
        printf("disk init error\r\n");  // disk initialization fail
        goto end;
    }

    if (!FAT_Initialize(&disk)) {
        printf("FAT init error\r\n");  // FAT initialization fail
        goto end;
    }

    // load kernel
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    if (!fd) {
        printf("Failed to open kernel\r\n");
        goto end;
    }
    uint32_t read;
    uint8_t* kernelBuffer = kernel;
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, kernelLoadBuffer))) {
        printf("read %u bytes\r\n", read);
        memcpy(kernelBuffer, kernelLoadBuffer, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);

    // execute kernel
    printf("Kernel found and preparing to load\r\n");
    KernelStart kernelStart = (KernelStart)kernel;
    printf("Preparing to load kernel at %x\r\n", kernel);
    kernelStart();
    printf("we're still in the bootloader, kernel didn't start\r\n");
    end:
        for(;;);
}
