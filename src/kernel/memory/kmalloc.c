#include <stdint.h>
#include <stdbool.h>
#include "memman.h"
#include "util/div.h"
#include "kmalloc.h"

static uint32_t g_heapStart, g_heapSize, g_threshold;
static bool g_kmallocInitialized = false;               // was memory allocated?

void Kmalloc_Init(uint32_t initialHeapSize) {
    g_heapStart = KERNEL_MALLOC;
    g_heapSize = 0;
    g_threshold = 0;
    g_kmallocInitialized = true;
    changeHeapSize(initialHeapSize);
}

void changeHeapSize(int newSize) {
    int oldPageTop = CEIL_DIV(g_heapSize, PAGE_SIZE);
    int newPageTop = CEIL_DIV(newSize, PAGE_SIZE);

    uint32_t pages = (newSize + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t allocated = 0;

    for (uint32_t i = 0; i < pages; ++i) {
        uint32_t phys = pmmAllocPageFrame();
        if (phys == 0) {
            /* rollback previously allocated frames */
            for (uint32_t j = 0; j < allocated; ++j) {
                uint32_t vaddr = KERNEL_MALLOC + g_heapSize + j * PAGE_SIZE;
                /* TODO: implement pmmFreePageFrame and unmap page; for now just stop */
                (void)vaddr;
            }
            return; // out of memory
        }

        uint32_t vaddr = KERNEL_MALLOC + g_heapSize + i * PAGE_SIZE;
        if (memMapPage((uint32_t*)vaddr, phys, PAGE_FLAG_WRITE) != 0) {
            /* memMapPage failed, rollback */
            for (uint32_t j = 0; j < allocated; ++j) {
                uint32_t rvaddr = KERNEL_MALLOC + g_heapSize + j * PAGE_SIZE;
                (void)rvaddr;
            }
            return; // rollback on failure
        }

        allocated++;
    }

    g_heapSize += pages * PAGE_SIZE;
}
