#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory.h"
#include "memory/memman.h"
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
    // multiboot gives physical addresses; convert to kernel virtual addresses
    // (kernel is mapped at KERNEL_START) before dereferencing
    uint8_t* mods_addr_virt = (uint8_t*)( (uintptr_t)mbinfo->mods_addr + KERNEL_START );
    uint32_t mod1 = *(uint32_t*)(mods_addr_virt + 4);
    uint32_t physicalAllocStart = (mod1 + 0xFFF) & ~0xFFF;
    HAL_Init(mbinfo->mem_upper * 1024, physicalAllocStart);
    printk("All hardware initialized\n");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Booted by Multiboot (magic ok)\n");

        // ensure multiboot buffers are mapped into the higher-half before use
        // map multiboot physical buffers (mmap, modules, drives) into the
        // kernel virtual space so dereferencing won't page-fault
        extern void map_physical_range(uint32_t phys_start, uint32_t length, uint32_t* allocStart);
        if (mbinfo->mmap_length && mbinfo->mmap_addr)
            map_physical_range((uint32_t)mbinfo->mmap_addr, mbinfo->mmap_length, &physicalAllocStart);
        if (mbinfo->mods_count && mbinfo->mods_addr)
            map_physical_range((uint32_t)mbinfo->mods_addr, mbinfo->mods_count * sizeof(uint32_t) * 2, &physicalAllocStart);
        if (mbinfo->drives_length && mbinfo->drives_addr)
            map_physical_range((uint32_t)mbinfo->drives_addr, mbinfo->drives_length, &physicalAllocStart);

        // print all detected memory
        parse_multiboot_memmap(mbinfo);
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

    // convert physical drive buffer pointer to kernel virtual
    uintptr_t addr = (uintptr_t)mbinfo->drives_addr + KERNEL_START;
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
    // multiboot mmap pointer is physical; convert to kernel virtual address
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr + KERNEL_START;
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
