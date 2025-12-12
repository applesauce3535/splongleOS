#include "include/kernel_includes.h"

void crash_me();

extern uint8_t __bss_start;
extern uint8_t __end;
extern uint8_t phys;

void DrawRect() {
    for (int32_t y = 0; y < 200; ++y) {
        for (int32_t x = 0; x < 320; ++x) {
            PutPixel(x, y, 0x00, 0x00, 0xA8);
        }
    }
}

void task1() {
    while(1) {
        printk("task1 ");
        sleep(1000);
        Schedule();
    }
}

void task2() {
    while(1) {
        printk("task2 ");
        sleep(1000);
        Schedule();
    }
}

void kernel_main(uint32_t magic, multiboot_info_t* mbinfo) {
    clrscr();
    printk("\n");
    // printk("All stuff initialized\n");
    // printk("Testing sleep function: ");
    // sleep(3000);
    // printk("Done\n");
    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Booted by Multiboot (magic ok)\n");
    }
    else {
        // something went wrong, display incorrect multiboot magic number
        printk("Not booted by Multiboot: magic=0x%x\n", magic);
    }

    uint64_t total_mem = get_total_mem(mbinfo);
    Memory_Manager_Init(MEMMAP_AREA, total_mem);
    set_type1(mbinfo);
    // set certain regions for kernel and mmap as used
    deinitialize_region(0x00100000, 0x00200000);
    deinitialize_region(MEMMAP_AREA, (total_mem / BLOCK_SIZE) / BLOCKS_PER_BYTE);
    if (Page_Manager_Init()) printk("Paging enabled\n");
    HAL_Init();
    Multitasking_Init();
    // // test page fault
    // volatile uint32_t* poop = (uint32_t*)0xDEADBEEF;
    // printk("Gonna write to poop and cause a page fault\n");
    // // printk("%d\n", *poop);      // kernel read PF
    // // printk("Just printed variable after page faulting it\n");
    // *poop = 1234;               // kernel write PF
    // printk("Just wrote to poop after page faulting it\n");
    printk("Hello world from splongleOS\n");



    // printk("Type 'help' to get started!\n");
    create_kernel_task(Shell_Run);
    // create_kernel_task(task1);
    // create_kernel_task(task2);
    

    // crash_me();

    while (1) {
        Schedule();
    }
}