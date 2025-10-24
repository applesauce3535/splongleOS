#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/asm_wrappers.h"
#include "arch/i686/irq.h"
#include "memory/physical_manager.h"
#include "memory/page.h"
#include "memory/memmap.h"
#include "arch/i686/i8254.h"
#include "multiboot.h"

void crash_me();

void timer(Registers* regs) {
    printk(".");
}

extern uint8_t __bss_start;
extern uint8_t __end;
extern uint8_t phys;

void kernel_main(uint32_t magic, multiboot_info_t* mbinfo) {
    clrscr();
    HAL_Init();
    printk("All stuff initialized\n");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Booted by Multiboot (magic ok)\n");
        uint64_t total_mem = get_total_mem(mbinfo);
        Memory_Manager_Init(MEMMAP_AREA, total_mem);
        get_block_info();
        set_type1(mbinfo);
        // set certain regions for kernel and mmap as used
        deinitialize_region(0x00100000, 0x00200000);
        deinitialize_region(MEMMAP_AREA, (total_mem / BLOCK_SIZE) / BLOCKS_PER_BYTE);
        get_block_info();
        if (Page_Manager_Init()) printk("Paging enabled\n");

        // test page fault
        volatile uint32_t* poop = (uint32_t*)0xDEADBEEF;
        printk("Gonna write variable and cause a page fault\n");
        // printk("%d\n", *poop);      // kernel read PF
        // printk("Just printed variable after page faulting it\n");
        *poop = 1234;               // kernel write PF
        printk("Just wrote variable after page faulting it\n");
        printk("Hello world from splongleOS\n");
        printk("There's a race condition when displaying CPU speed and typing and I'm not gonna fix it hehehe\n");
    } 
    else {
        // something went wrong, display incorrect multiboot magic number
        printk("Not booted by Multiboot: magic=0x%x\n", magic);
    }

    // now entering the realm of the kernel main loop...
    uint32_t last_ticks = 0;
    while (1) {
        uint32_t ticks = get_ticks();
        if (ticks - last_ticks >= 100) {
            last_ticks = ticks;
            print_CPU();
        }
    }


    // crash_me();

    end:
        for (;;);
}