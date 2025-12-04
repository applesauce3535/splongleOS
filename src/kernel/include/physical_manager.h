// provide functions to allocate, free, or handle blocks of physical memory

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "memory.h"
#include "physical_manager.h"

#define BLOCK_SIZE      4096        // 4kB
#define BLOCKS_PER_BYTE 8           // each bit in a byte manages 1 block of memory, so each byte manages 4096 * 8 bytes of memory
// takes the offset into the memory map and the exact bit to look for in offset
#define INDEX_MMAP(offset, bit)     (offset * 32 + bit)
#define MEMMAP_AREA     0x30000

// sets block/bit in mem map to used
void set_block(int32_t bit);

// unsets block/bit in mem map to free
void unset_block(uint32_t bit);

// check if a block/bit in the memory map is set/used
bool check_block(uint32_t bit);

// find the first free block of memory for a size
uint64_t find_first_free_block(uint32_t num_blocks);

// initialize memory manager given an address and size to put the mmap
void Memory_Manager_Init(uint32_t address, uint64_t size);

// initialize region of memory as free
void initialize_region(uint32_t base, uint64_t size);

// initialize region of memory as used
void deinitialize_region(uint32_t base, uint64_t size);

// alloc blocks and return phys address of beginning of allocation
uint32_t* allocate_blocks(uint32_t num_blocks);

// free blocks of memory
void free_blocks(uint32_t* address, uint32_t num_blocks);

// print the used and total blocks
void get_block_info();