#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "memory/physical_manager.h"
#include "memory/page.h"
#include "memory/memmap.h"
#include "dev/drive.h"
#include "multiboot.h"

void crash_me();

void timer(Registers* regs) {
    printk(".");
}

void test_map_fault() {
    void* frame = allocate_blocks(1);  // allocate 4KB physical frame
    if (!frame) {
        printk("Failed to allocate frame\n");
        return;
    }

    void* virt = (void*)0xDEADBEEF;

    if (!map_page(frame, virt)) {
        printk("map_page failed\n");
        return;
    }

    // should now be safe
    volatile uint32_t* poop = (volatile uint32_t*)virt;
    printk("mapped poop: %x to physical: %x\n", poop, frame);
    *poop = 1234;

    printk("Wrote value %u to mapped address %x successfully\n", *poop, poop);
}

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
        uint64_t total_mem = get_total_mem(mbinfo);
        Memory_Manager_Init(MEMMAP_AREA, total_mem);
        get_block_info();
        set_type1(mbinfo);
        // set certain regions for kernel and mmap as used
        deinitialize_region(0x00100000, 0x00200000);
        deinitialize_region(MEMMAP_AREA, (total_mem / BLOCK_SIZE) / BLOCKS_PER_BYTE);
        get_block_info();
        Page_Manager_Init();
        printk("Paging enabled\n");

        // test page fault
        // volatile uint32_t* poop = (uint32_t*)0xDEADBEEF;
        // *poop = 1234;

        // test page mapping
        test_map_fault();
        printk("Hello world from splongleOS\n");
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