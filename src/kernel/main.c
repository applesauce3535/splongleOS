#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "multiboot.h"

void crash_me();

void timer(Registers* regs) {
    printk(".");
}

void parse_multiboot_memmap(multiboot_info_t* mbinfo);
void parse_multiboot_driveinfo(multiboot_info_t* mbinfo);

extern uint8_t _kernel_end;
extern uint8_t phys;

void kernel_main(uint32_t magic, multiboot_info_t* mbinfo) {
    uint32_t mod1 = *(uint32_t*)(mbinfo->mods_addr + 4);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0xFFF;
    HAL_Init(mbinfo->mem_upper * 1024, physicalAllocStart);
    printk("All hardware initialized\n");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Booted by Multiboot (magic ok)\n");
        // int* poop = 0x0;
        // printk("%d", *poop);        // force page fault
        // print all detected memory
        // parse_multiboot_memmap(mbinfo);
        // also print memory occupied by kernel
        // printk("Kernel starts at 0x%x, ends at 0x%x, occupies 0x%x amount of space.\nDon't override this\n", &phys, &_kernel_end, &_kernel_end-&phys);
        // print drive info
        // parse_multiboot_driveinfo(mbinfo);
    } else {
        // something went wrong, display incorrect multiboot magic number
        printk("Not booted by Multiboot: magic=0x%x\n", magic);
    }

    // i686_IRQ_RegisterHandler(0, timer);

    // crash_me();

    end:
        for (;;);
}

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

void parse_multiboot_memmap(multiboot_info_t* mbinfo) {
    // check if the multiboot memory map failed
    if (!(mbinfo->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printk("No multiboot memory map; mem_lower=%u mem_upper=%u\n",
               mbinfo->mem_lower, mbinfo->mem_upper);
        return;
    }

    // get entire length of memeory map from GRUB
    uint32_t mmap_len = mbinfo->mmap_length;
    // get base address of mmap buffer
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr;
    // offset inside mmap buffer, increments by size of each entry
    uintptr_t offset = 0;

    while (offset < mmap_len) {
        // for each entry add offset to base mmap address
        multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mmap_addr + offset);
        // get the base address, length, and type (free/occupied) for each entry
        uint64_t base = entry->addr;
        uint64_t length = entry->len;
        uint32_t type = entry->type;

        // FIXME: size isn't printing correctly
        // print each entry nicely
        printk("mmap: base=0x%llx len=0x%llx size=0x%x type=%u\n", base, length, entry->size, type);

        // go to next record, entry->size is the bytes AFTER size field, so to get full entry we must
        // also add the size of entry->size
        offset += entry->size + sizeof(entry->size);
    }
}
