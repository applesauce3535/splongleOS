#pragma once

#include "arch/i686/isr.h"

void Keyboard_Init();
void keyboardHandler(Registers* regs);