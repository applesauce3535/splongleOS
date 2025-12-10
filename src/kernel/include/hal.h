#pragma once

#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "pic.h"
#include "i8254.h"
#include "rtc.h"
#include "keyboard.h"
// #include "include/mouse.h"
#include "pc_speaker.h"
#include "stdio.h"
#include "driver.h"
#include "pci.h"
#include "ata.h"
#include "vga.h"

void HAL_Init();