#pragma once

#include "isr.h"

typedef void (*IRQHandler)(Registers* regs);

void i686_IRQ_Init();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);