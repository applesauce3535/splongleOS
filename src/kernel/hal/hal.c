#include <stdio.h>
#include "hal.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/isr.h"
#include "arch/i686/irq.h"
#include "arch/i686/pic.h"
#include "arch/i686/i8254.h"
#include "arch/i686/rtc.h"
#include "dev/keyboard.h"
#include "dev/pc_speaker.h"
#include "stdio.h"



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
}