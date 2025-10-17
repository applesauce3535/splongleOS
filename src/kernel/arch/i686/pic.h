#pragma once

#include <stdint.h>

void i686_PIC_Config(uint8_t offsetPIC1, uint8_t offsetPIC2);
void i686_PIC_Mask(int irq);
void i686_PIC_Unmask(int irq);
void i686_PIC_SendEndOfInt(int irq);
void i686_PIC_Disable();
uint16_t i686_PIC_ReadIRQRequestRegister();
uint16_t i686_PIC_ReadInServiceRegister();