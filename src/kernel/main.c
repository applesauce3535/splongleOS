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
        lock_scheduler();
        Schedule();
        unlock_scheduler();
        block_task(PAUSED);
    }
}

void task2() {
    while(1) {
        printk("task2 ");
        sleep(1000);
        lock_scheduler();
        Schedule();
        unlock_scheduler();
    }
}

void task3() {
    while(1) {
        printk("task3 ");
        sleep(1000);
        lock_scheduler();
        Schedule();
        unlock_scheduler();
    }
}


void kernel_main(uint32_t magic, multiboot_info_t* mbinfo) {
    clrscr();
    printk("\n");
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
    printk("Multitasking enabled\n");
    // primary master/primary slave
    // use interrupt 14
    // AdvancedTechnologyAttachment ata0m = ATA_Init(0x1F0);
    // if (ata0m.initialized) printk("Master ATA1 initialized\n");
    // ATA_Identify(&ata0m);
    // char wbuffer[9] = "splongle";
    // ATA_Write28(10, wbuffer, 9, &ata0m);
    // ATA_Read28(10, 9, &ata0m);

    // AdvancedTechnologyAttachment ata0s = ATA_Init(0x1F0, false);
    // if (ata0s.initialized) printk("Slave ATA1 initialized\n");
    // Identify(&ata0s);
    // // secondary master/secondary slave
    // // use interrupt 15
    // AdvancedTechnologyAttachment ata1m = ATA_Init(0x170, true);
    // if (ata1m.initialized) printk("Master ATA2 initialized\n");
    // Identify(&ata1m);
    // AdvancedTechnologyAttachment ata1s = ATA_Init(0x170, false);
    // if (ata1s.initialized) printk("Slave ATA2 initialized\n");
    // Identify(&ata1s);
    // printk("Testing ATA write and read...\n");
    // char* wbuffer = "splongle";
    // char rbuffer[9];
    // Write28(10, (uint8_t*)wbuffer, 9, &ata0m);
    // Read28(10, (uint8_t*)rbuffer, 9, &ata0m);
    // printk("Write buffer = ");
    // for (uint8_t i = 0; i < 9; ++i) {
    //     printk("%c", wbuffer[i]);
    // }
    // printk("\nRead buffer = ");
    // for (uint8_t i = 0; i < 9; ++i) {
    //     printk("%c", rbuffer[i]);
    // }
    
    // if (memcmp((void*)wbuffer, (void*)rbuffer, 9) == 0) printk("Read/Write works!\n");

    // // test page fault
    // volatile uint32_t* poop = (uint32_t*)0xDEADBEEF;
    // // printk("Gonna write to poop and cause a page fault\n");
    // // printk("%d\n", *poop);      // kernel read PF
    // // printk("Just printed variable after page faulting it\n");
    // *poop = 1234;               // kernel write PF
    // printk("Just wrote to poop after page faulting it\n");
    printk("\nHello world from ");
    change_color(0x0B);
    printk("SplongleOS");
    default_color();
    printk("! :)\n");



    printk("Type 'help' to get started!\n");
    create_kernel_task(Shell_Run);
    // thread_control_block* t1 = create_kernel_task(task1);
    // thread_control_block* t2 = create_kernel_task(task2);
    // create_kernel_task(task3);
    // printk("Killing task 2\n");
    // kill_kernel_task(t2);
    

    // crash_me();

    while (1) {
        // printk("Acquiring lock\n");
        // acquire_lock();
        Schedule();
        // printk("Releasing lock\n");
        // release_lock();
        // unblock_task(t1);
    }
}