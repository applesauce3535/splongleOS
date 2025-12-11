#include "include/hal.h"

void HAL_Init() {
    // uncomment printk's for debugging
    i686_GDT_Initialize();
    // printk("GDT initialized\n");
    i686_IDT_Initialize();
    // printk("IDT initialized\n");
    i686_ISR_Initialize();
    // printk("ISR initialized\n");
    i686_IRQ_Init();
    // printk("IRQ initialized\n");
    // default PIT rate ~100Hz
    // 1193182 MHz / 11932 = ~100, so it's triggered *almost* 100 times each second (every 10 ms)
    set_PIT_channel_mode_frequency(0, 2, 11932);
    PIT_Init();
    // printk("PIT initialized\n");
    Keyboard_Init();
    // printk("Keyboard initialized\n");
    RTC_Init();
    enable_RTC();
    
    SelectDrivers(&g_Manager);
    
    // primary master/primary slave
    // use interrupt 14
    AdvancedTechnologyAttachment ata0m = ATA_Init(0x1F0, true);
    if (ata0m.initialized) printk("Master ATA1 initialized\n");
    AdvancedTechnologyAttachment ata0s = ATA_Init(0x1F0, false);
    if (ata0s.initialized) printk("Slave ATA1 initialized\n");

    // secondary master/secondary slave
    // use interrupt 15
    AdvancedTechnologyAttachment ata1m = ATA_Init(0x170, true);
    if (ata1m.initialized) printk("Master ATA2 initialized\n");
    AdvancedTechnologyAttachment ata1s = ATA_Init(0x170, false);
    if (ata1s.initialized) printk("Slave ATA2 initialized\n");
    // IDK WHY THE MOUSE DOESNT WORK
    // if (Mouse_Init()) printk("Mouse initialized\n");
    /*
    for more ATAs:
    third: starts at 0x1E8
    fourth: starts at 0x168
    */
//    printk("Preparing switch to graphics mode! (320x200, 8)\n");
//    sleep(5000);
//    clrscr();
//    VGA_Init();
}