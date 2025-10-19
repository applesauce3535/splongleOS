#include <stdint.h>
#include "util/div.h"
#include "memory.h"
#include "stdio.h"
#include "multiboot.h"
#include "memman.h"
#include "arch/i686/io.h"

#define NUM_PAGES_DIRS 256
#define NUM_PAGE_FRAMES (0x100000000 / PAGE_SIZE / 8)

static uint32_t g_pageFrameMin, g_pageFrameMax, g_totalAlloc;

uint8_t g_physicalMemoryBitmap[NUM_PAGE_FRAMES / 8];

// each page dir has 1024 entrys
static uint32_t g_pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(PAGE_SIZE)));    // align on pages

static uint8_t g_pageDirsUsed[NUM_PAGES_DIRS];

// memory can be mapped from starting point to end point
void Memory_Init(uint32_t memHigh, uint32_t allocStart) {
    // remapping VGA buffer
    const uint32_t vga_phys = 0x000B8000;
    const uintptr_t vga_virt = KERNEL_START + 0x000B8000;

    uint32_t pde_idx = (vga_virt >> 22) & 0x3FF;
    uint32_t pte_idx = (vga_virt >> 12) & 0x3FF;
    
    uint32_t phys_pd = (uint32_t)initial_page_dir - KERNEL_START;

    // if PDE not present, allocate one page for a page table at allocStart
    if (!(initial_page_dir[pde_idx] & PAGE_FLAG_PRESENT)) {
        uint32_t pt_phys = allocStart;          // use the physical page passed in as scratch
        allocStart += PAGE_SIZE;                // advance allocStart by a page so it won't be reused
        // & 0xFFFFF000 shaves off the lower 12 bits of a page address, because pages are only 4KB
        initial_page_dir[pde_idx] = (pt_phys & PAGE_MASK) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
        // zero the new page table through the kernel virtual mapping
        memset((void*)(KERNEL_START + pt_phys), 0, PAGE_SIZE);
    }

    // get virtual pointer to the new page table
    uint32_t pt_phys = initial_page_dir[pde_idx] & PAGE_MASK;
    uint32_t *pt = (uint32_t*)(KERNEL_START + pt_phys);

    // set PTE for VGA page
    pt[pte_idx] = (vga_phys & PAGE_MASK) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;

    // flush TLB for that virtual page
    i686_InvalidatePage((uint32_t)vga_virt);

    // invalidate certain pages
    initial_page_dir[0] = 0;
    i686_InvalidatePage(0);

    // recursive mapping, start at end of page directory
    initial_page_dir[1023] = phys_pd | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
    i686_InvalidatePage(PAGE_MASK);

    pmm_init(allocStart, memHigh);
    memset(g_pageDirs, 0, PAGE_SIZE * NUM_PAGES_DIRS);
    memset(g_pageDirsUsed, 0, NUM_PAGES_DIRS);
}

void pmm_init(uint32_t memLow, uint32_t memHigh) {
    // divide by size of a page -> 4kB
    g_pageFrameMin = CEIL_DIV(memLow, PAGE_SIZE);
    g_pageFrameMax = memHigh / PAGE_SIZE;
    g_totalAlloc = 0;
    memset(g_physicalMemoryBitmap, 0, sizeof(g_physicalMemoryBitmap));
}

/* 
    Map a physical range [phys_start, phys_start+length] into the kernel
    higher-half using pages allocated from allocStart. This is a simple
    helper used very early in boot when the full PMM isn't available.
    allocStart is updated to consume pages used for newly created page tables. 
*/
void map_physical_range(uint32_t phys_start, uint32_t length, uint32_t* allocStart) {
    uintptr_t start = phys_start & PAGE_MASK;
    uintptr_t end = ((phys_start + length + 0xFFF) & PAGE_MASK);

    for (uintptr_t i = start; i < end; i += PAGE_SIZE) {
        uintptr_t virt = (uintptr_t)KERNEL_START + i;
        uint32_t pde_idx = (virt >> 22) & 0x3FF;
        uint32_t pte_idx = (virt >> 12) & 0x3FF;

        // allocate a page table entry if not present
        if (!(initial_page_dir[pde_idx] & PAGE_FLAG_PRESENT)) {
            uint32_t pt_phys = *allocStart;
            *allocStart += PAGE_SIZE;
            initial_page_dir[pde_idx] = (pt_phys & PAGE_MASK) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
            memset((void*)(KERNEL_START + pt_phys), 0, PAGE_SIZE);
        }

        // get physical address
        uint32_t pt_phys = initial_page_dir[pde_idx] & PAGE_MASK;
        uint32_t* pt = (uint32_t*)(KERNEL_START + pt_phys);

        pt[pte_idx] = ((uint32_t)i & PAGE_MASK) | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
        i686_InvalidatePage((uint32_t)virt);
    }
}
