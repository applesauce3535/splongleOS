#pragma once

#include "isr.h"
#include "i8259.h"
#include "pic.h"
#include "asm_wrappers.h"
#include "stdio.h"
#include "util/arrays.h"
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20   // first 32 ISRs are CPU exceptions so start at offset 32 (0x20)

typedef void (*IRQHandler)(Registers* regs);

void i686_IRQ_Init();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);