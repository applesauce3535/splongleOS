#pragma once

#include <stdint.h>
#include "isr.h"
#include <stdint.h>
#include "stdio.h"
#include "asm_wrappers.h"
#include "isr.h"
#include "irq.h"
#include "sched.h"


void PIT_Init();
// change PIT channel frequency
void set_PIT_channel_mode_frequency(uint8_t channel, uint8_t mode, uint16_t freq);
uint32_t get_ticks();
void PIT_Handler(Registers* regs);
void sleep(uint32_t ms);