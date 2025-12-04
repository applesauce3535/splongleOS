#pragma once

#include "isr.h"
#include <stdint.h>
#include <stdbool.h>
#include "asm_wrappers.h"
#include "irq.h"
#include "stdio.h"

#define KEYBOARD_PORT 0x60

void Keyboard_Init();
void keyboardHandler(Registers* regs);
char keyboard_getchar();
bool keyboard_haschar();