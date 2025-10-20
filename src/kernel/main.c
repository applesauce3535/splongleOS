#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "memory/physical_manager.h"
#include "memory/memmap.h"
#include "dev/drive.h"
#include "multiboot.h"

void crash_me();

void timer(Registers* regs) {
    printk(".");
}

void parse_multiboot_driveinfo(multiboot_info_t* mbinfo);

extern uint8_t __bss_start;
extern uint8_t __end;
extern uint8_t phys;

void kernel_main(uint32_t magic, multiboot_info_t* mbinfo) {
    HAL_Init();
    printk("All hardware initialized\n");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Booted by Multiboot (magic ok)\n");
        // print all detected memory
        // parse_multiboot_memmap(mbinfo);
        // also print memory occupied by kernel
        // printk("Kernel starts at 0x%x, ends at 0x%x, occupies 0x%x amount of space.\nDon't override this\n", &phys, &__end, &__end-&phys);
        // print drive info
        // parse_multiboot_driveinfo(mbinfo);
        uint32_t total_mem = get_total_mem(mbinfo);
        Memory_Manager_Init(MEMMAP_AREA, total_mem);
        get_block_info();
        set_type1(mbinfo);
        // set certain regions for kernel and mmap as used
        deinitialize_region(0x00100000, 0x00200000);
        deinitialize_region(MEMMAP_AREA, (total_mem / BLOCK_SIZE) / BLOCKS_PER_BYTE);
        get_block_info();
    } 
    else {
        // something went wrong, display incorrect multiboot magic number
        printk("Not booted by Multiboot: magic=0x%x\n", magic);
    }

    // i686_IRQ_RegisterHandler(0, timer);

    // crash_me();

    end:
        for (;;);
}