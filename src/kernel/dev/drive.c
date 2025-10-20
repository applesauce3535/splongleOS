#include <stdint.h>
#include "stdio.h"
#include "multiboot.h"
#include "drive.h"

void parse_multiboot_driveinfo(multiboot_info_t* mbinfo) {
    // check if the drive info failed
    if (!(mbinfo->flags & MULTIBOOT_INFO_DRIVE_INFO)) {
        printk("ERROR: No drive info available\n");
        return;
    }

    /* get the full range of info for the drive list. Use uintptr_t so
       pointer arithmetic is correct on the target. */
    uintptr_t addr = (uintptr_t)mbinfo->drives_addr;
    uintptr_t end = addr + (uintptr_t)mbinfo->drives_length;

    while (addr < end) {
        // recast addr as a pointer to drive struct and assign to drive
        multiboot_drive_t* drive = (multiboot_drive_t*)addr;

        // print drive info nicely
        printk("Drive 0x%x: mode=%d, C/H/S: %d/%d/%d\n",
            drive->drive_number,
            drive->drive_mode,
            drive->drive_cylinders,
            drive->drive_heads,
            drive->drive_sectors);

        // advance to the next drive
        if (drive->size == 0) {
            printk("ERROR: drive entry with size=0, aborting parse\n");
            break;
        }
        addr += (uintptr_t)drive->size;
    }
}