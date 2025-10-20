#pragma once
#include <stdint.h>

#define KERNEL_START        0xC0000000
#define KERNEL_MALLOC       0xD0000000

#define REC_PAGEDIR         ((uint32_t*)0xFFFFF000)
#define REC_PAGETABLE(i)    ((uint32_t*) (0xFFC00000 + ((i) << 12))) 

#define PAGE_SIZE           4096U
#define PAGE_MASK           0xFFFFF000U
#define PAGE_FLAGS_MASK     0x00000FFFU
#define PAGE_ALIGN_DOWN(x)  ((x) & PAGE_MASK)
#define PTE_MAKE_ENTRY(phys, flags) (((phys) & PAGE_MASK) | ((flags) & PAGE_FLAGS_MASK))
#define PTE_GET_PHYS(entry) ((entry) & PAGE_MASK)

typedef enum {
    PAGE_FLAG_PRESENT           = 0x1,
    PAGE_FLAG_WRITE             = 0x2,
    PAGE_FLAG_OWNER             = 0x200
} PAGE_FLAGS;

extern uint32_t initial_page_dir[1024];

void pmm_init(uint32_t memLow, uint32_t memHigh);
uint32_t pmmAllocPageFrame();
int memMapPage(uint32_t virtualAddr, uint32_t physAddr, uint32_t flags);
uint32_t* memGetCurrentPageDir();
void memChangePageDir(uint32_t* pd_virt);
void syncPageDirs();
void Memory_Init(uint32_t memHigh, uint32_t allocStart);