#pragma once

#include <stdint.h>
#include "multiboot.h"


// print the memory map nicely
void parse_multiboot_memmap(multiboot_info_t* mbinfo);
// get the total amount of memory
uint64_t get_total_mem(multiboot_info_t* mbinfo);
// get the number of memory entries
uint32_t get_num_entries(multiboot_info_t* mbinfo);
// set all type 1 regions as free
void set_type1(multiboot_info_t* mbinfo);
// print memory info to top of screen
void print_mem();