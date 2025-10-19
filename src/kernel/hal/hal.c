#include <stdio.h>
#include "hal.h"
#include "memory/memman.h"
#include "arch/i686/gdt.h"
#include "arch/i686/idt.h"
#include "arch/i686/isr.h"
#include "arch/i686/irq.h"
#include "arch/i686/pic.h"
#include "dev/keyboard.h"
#include "stdio.h"



void HAL_Init(uint32_t memHigh, uint32_t allocStart) {
    i686_GDT_Initialize();
    printk("GDT initialized\n");
    i686_IDT_Initialize();
    printk("IDT initialized\n");
    i686_ISR_Initialize();
    printk("ISR initialized\n");
    i686_IRQ_Init();
    printk("IRQ initialized\n");
    Keyboard_Init();
    printk("Keyboard initialized\n");
    Memory_Init(memHigh, allocStart);
    printk("Memory initialized\n");
}