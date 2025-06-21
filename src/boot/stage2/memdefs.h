#pragma once

// already occupied memory spaces (from OS dev wiki)
// 0x00000000 - 0x000003FF = interrupt vector table
// 0x00000400 - 0x000004FF = BIOS data start

// valid memory range
#define MEMORY_MIN 0x00000500
#define MEMORY_MAX 0x00080000

// 0x0000500 - 0x00010500 = FAT driver location
#define MEMORY_FAT_ADDR ((void far*)0x00500000) // segment:offset (SSSS:OOOO)
#define MEMORY_FAT_SIZE 0x00010500              // 64 kB

// occupied memory past valid range
// 0x00080000 - 0x0009FFFF = extended BIOS data area
// 0x000A0000 - 0x000C7FFF = video
// 0x000C8000 - 0x000FFFFF = BIOS