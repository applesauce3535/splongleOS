#pragma once

#include "pic.h"
#include "i8259.h"
#include "asm_wrappers.h"

#define PIC1_COMMAND_PORT   0x20
#define PIC1_DATA_PORT      0x21
#define PIC2_COMMAND_PORT   0xA0
#define PIC2_DATA_PORT      0xA1

const PICDriver* i8259_GetDriver();