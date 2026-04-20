#include "include/page.h"

page_directory* g_currentPD = 0;

pt_entry* get_pte(page_table* pt, virt_addr virt) {
    if (pt) return &pt->entries[PT_INDEX(virt)];
    return 0;
}

pd_entry* get_pde(page_directory* pd, virt_addr virt) {
    // we use the PT index to also get the PD index, because they are both 4KB
    if (pd) return &pd->entries[PT_INDEX(virt)];
    return 0;
}

pt_entry* get_page(virt_addr virt) {
    // get PD
    page_directory* pd = g_currentPD;
    // get PT in directory
    pd_entry* pde = &pd->entries[PD_INDEX(virt)];   // now we can use PD index, because we're indexing the full 4MB space
    page_table* pte = (page_table*)PAGE_PHYS_ADDR(pde);
    // get page
    pt_entry* page = &pte->entries[PT_INDEX(virt)];
    return page;
}

void* allocate_page(pt_entry* page) {
    void* block = allocate_blocks(1);   // a page and a block are the same size
    if (block) {
        // map the page to the block
        SET_FRAME(page, (phys_addr)block);
        SET_ATTR(page, PTE_PRESENT);
    }
    return block;
}

void free_page(pt_entry* page) {
    void* address = (void*)PAGE_PHYS_ADDR(page);
    if (address) free_blocks(address, 1);

    CLEAR_ATTR(page, PTE_PRESENT);
}

bool set_pd(page_directory* pd) {
    if (!pd) return false;

    g_currentPD = pd;
    // CR3 holds address of current PD
    __asm__ __volatile__ ("movl %%EAX, %%CR3"::"a"(g_currentPD));
    return true;
}

void flush_tlb_entry(virt_addr virt) {
    i686_InvalidatePage(virt);
}

bool map_page(void* phys, void* virt) {
    // get page
    page_directory* pd = g_currentPD;
    // get PT
    pd_entry* pde = &pd->entries[PD_INDEX((uint32_t)virt)];

    // is it present?
    if ((*pde & PDE_PRESENT) != PDE_PRESENT) {
        // get that thang some dind dang memory
        page_table* table = (page_table*)allocate_blocks(1);
        if (!table) return false;   // oops, out of memory

        // clear the table
        memset(table, 0, sizeof(page_table));

        // make new entry because we're mapping a new page
        pd_entry* pde = &pd->entries[PD_INDEX((uint32_t)virt)];
        // map in table + enable attributes
        SET_ATTR(pde, PDE_PRESENT | PDE_READ_WRITE);
        SET_FRAME(pde, (phys_addr)table);
    }
    // get PT phys
    page_table* pt = (page_table*)PAGE_PHYS_ADDR(pde);
    // get the page
    pt_entry* page = &pt->entries[PT_INDEX((uint32_t)virt)];
    // map in page
    SET_ATTR(page, PTE_PRESENT);
    SET_FRAME(page, (uint32_t)phys);
    flush_tlb_entry((virt_addr)virt);
    return true;    // success!
}

void unmap_page(void* virt) {
    pt_entry* page = get_page((uint32_t)virt);
    SET_FRAME(page, 0); // set phys to 0 (bad pointer)
    CLEAR_ATTR(page, PTE_PRESENT);
}

bool Page_Manager_Init(void) {
    // make default PD
    page_directory* dir = (page_directory*)allocate_blocks(3);
    if (!dir) return false; // oops, out of memory

    // clear PD and mark as current PD
    memset(dir, 0, sizeof(dir));
    for (uint32_t i = 0; i < 1024; ++i) dir->entries[i] = 0x2;  //  supervisor, read/write, not present
    // make default PT
    page_table* table = (page_table*)allocate_blocks(1);
    if (!table) return false; // oops, out of memory
    // make 3GB table
    page_table* table3G = (page_table*)allocate_blocks(1);
    if (!table3G) return false; // oops, out of memory
    // clear the tables
    memset(table, 0, sizeof(table));
    memset(table3G, 0, sizeof(table3G));
    // identity map first 4MB
    for (uint32_t i = 0, frame = 0x0, virt = 0x0; i < 1024; ++i, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        pt_entry page = 0;
        SET_ATTR(&page, PTE_PRESENT | PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        // add page to 3GB PT, we're basically putting all the BIOS crap and whatever in the kernel
        table3G->entries[PT_INDEX(virt)] = page;

    }
    // map kernel to 3GB+ (higher half kernel)
    for (uint32_t i = 0, frame = KERNEL_ADDRESS, virt = 0xC0000000; i < 1024; ++i, frame += PAGE_SIZE, virt += PAGE_SIZE) {
        pt_entry page = 0;
        SET_ATTR(&page, PTE_PRESENT | PTE_READ_WRITE);
        SET_FRAME(&page, frame);

        table->entries[PT_INDEX(virt)] = page;
    }

    // get PT in PD that refers to 3GB
    pd_entry* pde = &dir->entries[PD_INDEX(0xC0000000)];
    SET_ATTR(pde, PDE_PRESENT | PDE_READ_WRITE);
    // physically higher half kernel is mapped to lower memory
    SET_FRAME(pde, (phys_addr)table);

    pd_entry* pde2 = &dir->entries[PD_INDEX(0x00000000)];
    SET_ATTR(pde2, PDE_PRESENT | PDE_READ_WRITE);
    SET_FRAME(pde2, (phys_addr)table3G);

    // switch to PD
    set_pd(dir);

    // enable paging: set PG (bit 31) of CR0
    i686_EnablePaging();
    i686_ISR_RegisterHandler(14, &PFHandler);
    return true;


}

void PFHandler(Registers* regs) {
    uint32_t bad_addr = i686_ReadCR2();
    uint32_t ec = regs->error_code;

    bool not_present = !(ec & 1);
    bool write = (ec & 2);
    bool user = (ec & 4);

    // debugging info
    printk("Page fault at 0x%x, EC: 0x%x\n", bad_addr, ec);
    printk("  Caused by: %s in %s mode during %s\n",
        (ec & 0x1) ? "protection violation (page present)" : "non-present page",
        (ec & 0x4) ? "user" : "kernel",
        (ec & 0x2) ? "write" : "read");

    if (ec & 0x8)
        printk("  Reserved bit violation in page directory/table!\n");
    if (ec & 0x10)
        printk("  Caused by instruction fetch.\n");

    if (not_present && !user) {
        void* frame = allocate_blocks(1);
        if (frame) {        // fail? out of memory, we will have to kick out a page later
            uintptr_t vpage = bad_addr & ~0xFFF;
            if (map_page(frame, (void*)vpage)) {
                // printk("Kernel non-present page fault resolved, resuming execution\n");
                return; // resume
            }
        }
    }

    if (!not_present && user) {
        // this is a serious violation of the law! kill this man!
    }
    // I'm just covering kernel needing non-present page right now
    // if we reach this point, something went terribly wrong
    dump_regs(regs);
    for(;;);
}