#pragma once

#define i686_GDT_CODE_SEGMENT 0x08
#define i686_GDT_DATA_SEGMENT 0x10

#define ASMCALL __attribute__((cdecl))
void i686_GDT_Initialize();