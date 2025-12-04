#include "include/memmap.h"


void parse_multiboot_memmap(multiboot_info_t* mbinfo) {
    // check if the multiboot memory map failed
    if (!(mbinfo->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printk("No multiboot memory map; mem_lower=%u mem_upper=%u\n",
               mbinfo->mem_lower, mbinfo->mem_upper);
        return;
    }

    uint64_t free_mem = 0, total_mem = 0;

    // get entire length of memeory map from GRUB
    uint32_t mmap_len = mbinfo->mmap_length;
    // get base address of mmap buffer
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr;
    // offset inside mmap buffer, increments by size of each entry
    uintptr_t offset = 0;

    while (offset < mmap_len) {
        // for each entry add offset to base mmap address
        multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mmap_addr + offset);
        // get the base address, length, and type (free/occupied) for each entry
        uint64_t base = entry->addr;
        uint64_t length = entry->len;
        uint32_t type = entry->type;

        total_mem += length;

        if (type == 1) free_mem += length;

        // print each entry nicely
        printk("mmap: base=0x%llx len=0x%llx type=%u\n", base, length, type);

        // go to next record, entry->size is the bytes AFTER size field, so to get full entry we must
        // also add the size of entry->size
        offset += entry->size + sizeof(entry->size);
    }
    printk("Total blocks: %llu\nTotal used blocks: %llu\nTotal free blocks: %llu\n", \
        (unsigned long long)(total_mem / 4096), (unsigned long long)((total_mem - free_mem) / 4096), (unsigned long long)(free_mem / 4096));
}

uint64_t get_total_mem(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printk("No multiboot memory map; mem_lower=%u mem_upper=%u\n",
               mbinfo->mem_lower, mbinfo->mem_upper);
        return 0;
    }
    uint64_t total_mem = 0;
    uint32_t mmap_len = mbinfo->mmap_length;
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr;
    uintptr_t offset = 0;
    while (offset < mmap_len) {
        multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mmap_addr + offset);
        uint64_t base = entry->addr;
        uint64_t length = entry->len;
        uint32_t type = entry->type;
        total_mem += length;
        offset += entry->size + sizeof(entry->size);
    }
    return total_mem;
}

uint32_t get_num_entries(multiboot_info_t* mbinfo) {
    if (!(mbinfo->flags & MULTIBOOT_INFO_MEM_MAP)) {
        printk("No multiboot memory map; mem_lower=%u mem_upper=%u\n",
               mbinfo->mem_lower, mbinfo->mem_upper);
        return 0;
    }
    uint64_t num_entries = 0;
    uint32_t mmap_len = mbinfo->mmap_length;
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr;
    uintptr_t offset = 0;
    while (offset < mmap_len) {
        multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mmap_addr + offset);
        uint64_t base = entry->addr;
        uint64_t length = entry->len;
        uint32_t type = entry->type;
        ++num_entries;
        offset += entry->size + sizeof(entry->size);
    }
    return num_entries;
}

void set_type1(multiboot_info_t* mbinfo) {
    uint32_t mmap_len = mbinfo->mmap_length;
    uintptr_t mmap_addr = (uintptr_t)mbinfo->mmap_addr;
    uintptr_t offset = 0;

    while (offset < mmap_len) {
        
        multiboot_memory_map_t* entry = (multiboot_memory_map_t*)(mmap_addr + offset);
        uint64_t base = entry->addr;
        uint64_t length = entry->len;
        uint64_t end = base + length;
        uint32_t type = entry->type;
        offset += entry->size + sizeof(entry->size);
        // skip anything above 4GB
        if (base >= 0x100000000ULL) continue;

        // clip if it crosses 4GB boundary
        if (end > 0x100000000ULL) length = 0x100000000ULL - base;

        if (type == MULTIBOOT_MEMORY_AVAILABLE) initialize_region((uint32_t)base, length);

        else deinitialize_region((uint32_t)base, length);
    }
}

void print_mem() {
    int X = getX();
    int Y = getY();
    setX(0);
    setY(0);
    setcursor(0, 0);
    get_block_info();
    setX(X);
    setY(Y);
    setcursor(X, Y);
}