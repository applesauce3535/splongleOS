#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory.h"
#include "physical_manager.h"

static uint32_t* g_memoryMap = 0;
static uint32_t g_maxBlocks = 0;
static uint32_t g_usedBlocks = 0;


void set_block(uint32_t bit) {
    // offset into map by dividing by 32, then shift 1 by passed bit modulo 32 to get the exact bit in the offset
    // finally OR it with the memory map to set the block as being used
    g_memoryMap[bit/32] |= (1 << (bit % 32));
}

void unset_block(uint32_t bit) {
    // almost the same as above, but ANDing the negation of the shift with the memory map to mark block as free
    // (negating the result will allow the memory map to retain all currently used blocks and only free the passed block)
    g_memoryMap[bit/32] &= ~(1 << (bit % 32));
}

bool check_block(uint32_t bit) {
    // true: block is used; false: block is free
    return g_memoryMap[bit/32] & (1 << (bit % 32));
}

int32_t find_first_free_block(uint32_t num_blocks) {
    if (num_blocks == 0) return 0;      // why would someone request 0 memory? are they dumb?

    // test 32 blocks at a time
    for (uint32_t i = 0; i < g_maxBlocks / 32; ++i) {
        // check if there is at least some free block
        if (g_memoryMap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; ++j) {
                // get the exact block (a single bit) we are currently checking
                uint32_t bit = 1 << j;
                // if bit is unset, start of free region
                if (!(g_memoryMap[i] & bit)) {
                    uint32_t start_bit = INDEX_MMAP(i, bit);  // indexing memory map
                    uint32_t free_blocks = 0;

                    for (uint32_t count = 0; count <= num_blocks; ++count) {
                        if (!check_block(start_bit + count)) free_blocks++;

                        // check if we've found enough space and return beginning of region
                        if (free_blocks == num_blocks) return INDEX_MMAP(i, j);
                    }
                }
            }
        }
    }

    // no free space large enough, oops!
    return -1;
}

void Memory_Manager_Init(uint32_t address, uint64_t size) {
    // limit 4GB addressable phys memory
    if ((uint64_t)size >= 0x100000000) {
        size = 0xFFFFFFFF;
    }
    // place the map at the address
    g_memoryMap = (uint32_t*)address;

    g_maxBlocks = size / BLOCK_SIZE;
    g_usedBlocks = g_maxBlocks;         // every block will be set as used initially
    // set all of mmap to 1
    memset(g_memoryMap, 0xFF, g_maxBlocks / BLOCKS_PER_BYTE);
}

void initialize_region(uint32_t base, uint64_t size) {
    if ((uint64_t)base + size >= 0x100000000ULL) {
        size = 0xFFFFFFFF - base;
    }
    // convert phys address to blocks
    uint32_t align = base / BLOCK_SIZE;

    // convert phys size to blocks
    uint32_t num_blocks = size / BLOCK_SIZE;

    for (; num_blocks > 0; num_blocks--) {
        unset_block(align++);
        g_usedBlocks--;
    }

    // always set 1st block to not overwrite IDT, BIOS areas...
    set_block(0);
}

void deinitialize_region(uint32_t base, uint32_t size) {
    // convert phys address to a block
    uint32_t align = base / BLOCK_SIZE;

    // convert phys size to blocks
    uint32_t num_blocks = size / BLOCK_SIZE;

    for (; num_blocks > 0; num_blocks--) {
        set_block(align++);
        g_usedBlocks++;
    }
}

uint32_t* allocate_blocks(uint32_t num_blocks) {
    uint32_t free_blocks = g_maxBlocks - g_usedBlocks;
    if (free_blocks < num_blocks) {
        // not enough blocks
        return 0;
    }
    uint32_t starting_block = find_first_free_block(num_blocks);

    // check if there was a large enough region
    if (starting_block == -1) return 0;

    // found enough blocks, set as used
    for (uint32_t i = 0; i < num_blocks; ++i) set_block(starting_block + i);

    g_usedBlocks += num_blocks;

    // convert blocks to phys address
    uint32_t address = starting_block * BLOCK_SIZE;

    return (uint32_t*)address;
}

void free_blocks(uint32_t* address, uint32_t num_blocks) {
    // convert phys address to blocks
    uint32_t starting_block = (uint32_t)address / BLOCK_SIZE;

    for (uint32_t i = 0; i < num_blocks; ++i) unset_block(starting_block + i);

    g_usedBlocks -= num_blocks;
}

void get_block_info() {
    printk("Total blocks: %u, Used blocks: %u, Free blocks: %u\n", g_maxBlocks, g_usedBlocks, (g_maxBlocks - g_usedBlocks));
}