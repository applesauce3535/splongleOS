#include <stdint.h>
#include <stdbool.h>
#include "util/div.h"
#include "memory.h"
#include "stdio.h"
#include "multiboot.h"
#include "memman.h"
#include "arch/i686/io.h"

#define NUM_PAGES_DIRS 256
// number of 4KiB frames in 4GiB
#define NUM_PAGE_FRAMES ((uint64_t)0x100000000ULL / (uint64_t)PAGE_SIZE)
// bitmap bytes: one bit per frame 
#define PHYS_BITMAP_BYTES (NUM_PAGE_FRAMES / 8)

static uint32_t g_pageFrameMin, g_pageFrameMax, g_totalAlloc, g_memNumVPages;

uint8_t g_physicalMemoryBitmap[PHYS_BITMAP_BYTES];

// each page dir has 1024 entrys
static uint32_t g_pageDirs[NUM_PAGES_DIRS][1024] __attribute__((aligned(PAGE_SIZE)));    // align on pages

static uint8_t g_pageDirsUsed[NUM_PAGES_DIRS];

// memory can be mapped from starting point to end point
void Memory_Init(uint32_t memHigh, uint32_t allocStart) {
    g_memNumVPages = 0;
    // remapping VGA buffer
    const uint32_t vga_phys = 0x000B8000;
    const uintptr_t vga_virt = KERNEL_START + 0x000B8000;

    uint32_t pde_idx = (vga_virt >> 22) & 0x3FF;
    uint32_t pte_idx = (vga_virt >> 12) & 0x3FF;
    
    uint32_t phys_pd = (uint32_t)initial_page_dir - KERNEL_START;

    // if PDE not present, allocate one page for a page table at allocStart
    if (!(initial_page_dir[pde_idx] & PAGE_FLAG_PRESENT)) {
        uint32_t pt_phys = allocStart;      // use the physical page passed in as scratch
        allocStart += PAGE_SIZE;                 // advance allocStart by a page so it won't be reused
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

uint32_t pmmAllocPageFrame() {
    // iterate frame numbers from g_pageFrameMin (inclusive) to g_pageFrameMax (exclusive)
    uint32_t first = g_pageFrameMin;
    uint32_t last = g_pageFrameMax;

    for (uint32_t frame = first; frame < last; ++frame) {
        uint32_t byte_idx = frame / 8;
        uint32_t bit_idx = frame % 8;
        uint8_t mask = (uint8_t)(1u << bit_idx);

        if ((g_physicalMemoryBitmap[byte_idx] & mask) == 0) {
            // mark used
            g_physicalMemoryBitmap[byte_idx] |= mask;
            g_totalAlloc++;
            return frame * PAGE_SIZE;
        }
    }

    printk("pmmAllocPageFrame: out of frames\n");
    return 0;
}

uint32_t* memGetCurrentPageDir() {
    uint32_t cr3 = i686_GetPage();      // physical cr3 value
    uint32_t phys_pd = cr3 & PAGE_MASK; // mask low 12 bits to get physical address

    // add kernel start due to using high kernel
    return (uint32_t*)((uintptr_t) phys_pd + (uintptr_t)KERNEL_START);
}

void memChangePageDir(uint32_t* pd_virt) {
    uintptr_t phys = (uintptr_t)pd_virt - (uintptr_t)KERNEL_START;
    phys &= PAGE_MASK;      // get page aligned physical base
    // disable interrupts briefly while switching page directories
    i686_DisableInts();
    i686_ChangePage((uint32_t)phys);
    i686_EnableInts();
}

void syncPageDirs() {
    for (int idx = 0; idx < NUM_PAGES_DIRS; ++idx) {
        if (g_pageDirsUsed[idx]) {
            uint32_t* pageDir = g_pageDirs[idx];

            for (int j = 768; j < 1023; ++j) {
                pageDir[j] = initial_page_dir[j] & ~PAGE_FLAG_OWNER;
            }
        }
    }
}
int memMapPage(uint32_t virtualAddr, uint32_t physAddr, uint32_t flags) {
    uint32_t* prevPageDir = 0;

    if (virtualAddr >= KERNEL_START) {
        prevPageDir = memGetCurrentPageDir();
        if (prevPageDir != initial_page_dir) {
            memChangePageDir(initial_page_dir);
        }
    }

    uint32_t pdIndex = virtualAddr >> 22;
    uint32_t ptIndex = (virtualAddr >> 12) & 0x3FF;

    uint32_t* pageDir = REC_PAGEDIR;
    uint32_t* pageTable = REC_PAGETABLE(pdIndex);

    if (!(pageDir[pdIndex] & PAGE_FLAG_PRESENT)) {
        uint32_t ptPAddr = pmmAllocPageFrame();     // allocate a page frame if not present
        if (ptPAddr == 0) {
            if (prevPageDir != 0 && prevPageDir != initial_page_dir) {
                memChangePageDir(prevPageDir);
            }
            return -1; /* allocation failed */
        }

        // initialize page directory with address and flags
        pageDir[pdIndex] = ptPAddr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE | PAGE_FLAG_OWNER | flags;
        i686_InvalidatePage(virtualAddr);

        // initialize page table to 0
        for (uint32_t k = 0; k < 1024; ++k) {
            pageTable[k] = 0;
        }
    }

    pageTable[ptIndex] = physAddr | PAGE_FLAG_PRESENT | flags;
    g_memNumVPages++;
    i686_InvalidatePage(virtualAddr);

    if (prevPageDir != 0) {
        syncPageDirs();
        if (prevPageDir != initial_page_dir) {
            memChangePageDir(prevPageDir);
        }
    }

    return 0;
}