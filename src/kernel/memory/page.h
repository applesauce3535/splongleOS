// provide functions for paging, virt memory, mapping/unmapping pages...

#pragma once

#include <stdint.h>
#include<stdbool.h>
#include "physical_manager.h"
#include "arch/i686/isr.h"

#define PAGES_PER_TABLE         1024
#define TABLES_PER_DIRECTORY    1024
#define PAGE_SIZE               4096

#define KERNEL_ADDRESS          0x00100000

#define PD_INDEX(virt)      ((virt) >> 22 & 0x3FF)      // bits 22-31 specify index of PD entry
#define PT_INDEX(virt)      (((virt) >> 12) & 0x3FF)    // bits 12-21 specify index of PT entry
#define PAGE_PHYS_ADDR(pde) ((*pde) & ~0xFFF)           // clear low 12 bits
#define SET_ATTR(entry, attr)   (*entry |= attr)
#define CLEAR_ATTR(entry, attr) (*entry &= ~attr)
#define CHECK_ATTR(entry, attr) (*entry & attr)
// this only sets frame bits, the entry will retain all flags
#define SET_FRAME(entry, address)   (*entry = (*entry & ~0xFFFFF000) | address)

typedef uint32_t pt_entry;
typedef uint32_t pd_entry;
typedef uint32_t phys_addr;
typedef uint32_t virt_addr;

typedef enum {
    PTE_PRESENT         = 0x1,
    PTE_READ_WRITE      = 0x2,
    PTE_USER            = 0x4,          // 0 for kernel, 1 for user space
    PTE_WRITE_THROUGH   = 0x8,
    PTE_CACHE_DISABLE   = 0x10,
    PTE_ACCESSED        = 0x20,         // auto set by CPU
    PTE_DIRTY           = 0x40,
    PTE_PAT             = 0x80,
    PTE_GLOBAL          = 0x100,
    PTE_FRAME           = 0xFFFFF000    // phys address, mask on entry to get it
} PAGE_TABLE_FLAGS;

typedef enum {
    PDE_PRESENT         = 0x1,
    PDE_READ_WRITE      = 0x2,
    PDE_USER            = 0x4,          // 0 for kernel, 1 for user space
    PDE_WRITE_THROUGH   = 0x8,
    PDE_CACHE_DISABLE   = 0x10,
    PDE_ACCESSED        = 0x20,         // auto set by CPU
    PDE_DIRTY           = 0x40,         // 4MB entry only
    PDE_PAGE_SIZE       = 0x80,         // 0 = 4KB; 1 = 4MB
    PDE_GLOBAL          = 0x100,        // 4MB entry only
    PDE_PAT             = 0x2000,       // 4MB entry only
    PDE_FRAME           = 0xFFFFF000    // phys address, mask on entry to get it
} PAGE_DIR_FLAGS;

// each table handles 4MB
typedef struct {
    pt_entry entries[PAGES_PER_TABLE];
} __attribute__((aligned(4096))) page_table;

// each directory handles 4GB
typedef struct {
    pd_entry entries[TABLES_PER_DIRECTORY];
} __attribute__((aligned(4096))) page_directory;

// get entry in page table for an address
pt_entry* get_pte(page_table* pt, virt_addr virt);
// get entry in page directory for an address
pd_entry* get_pde(page_directory* pd, virt_addr virt);
// return a page (pointer) for a given virt address in the current pd
pt_entry* get_page(virt_addr virt);
// allocate a page
void* allocate_page(pt_entry* page);
// free a page
void free_page(pt_entry* page);
// set the current PD
bool set_pd(page_directory* pd);
// flush a page from TLB
void flush_tlb_entry(virt_addr virt);
// map a page
bool map_page(void* phys, void* virt);
// unmap a page
void unmap_page(void* virt);
// initialize virt memory manager
bool Page_Manager_Init(void);

void PFHandler(Registers* regs);