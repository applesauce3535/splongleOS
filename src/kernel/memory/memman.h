#pragma once
#include <stdint.h>

#define KERNEL_START 0xC0000000
#define PAGE_SIZE 4096U
#define PAGE_MASK 0xFFFFF000U
#define PAGE_FLAGS_MASK 0x00000FFFU
#define PAGE_ALIGN_DOWN(x) ((x) & PAGE_MASK)
#define PTE_MAKE_ENTRY(phys, flags) (((phys) & PAGE_MASK) | ((flags) & PAGE_FLAGS_MASK))
#define PTE_GET_PHYS(entry) ((entry) & PAGE_MASK)

typedef enum {
    PAGE_FLAG_PRESENT           = 0x1,
    PAGE_FLAG_WRITE             = 0x2
} PAGE_FLAGS;

extern uint32_t initial_page_dir[1024];

void map_physical_range(uint32_t phys_start, uint32_t length, uint32_t* allocStart);
void pmm_init(uint32_t memLow, uint32_t memHigh);
void Memory_Init(uint32_t memHigh, uint32_t allocStart);