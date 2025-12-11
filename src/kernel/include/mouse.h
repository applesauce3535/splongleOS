#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "asm_wrappers.h"
#include "irq.h"
#include "stdio.h"
#include "isr.h"

#define MOUSE_COMMAND   0x64
#define MOUSE_DATA      0x60

bool Mouse_Init();
void mouse_handler(Registers* regs);