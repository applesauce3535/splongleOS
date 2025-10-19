#include "gdt.h"
#include <stdint.h>

typedef struct {
    uint16_t SegmentLimitLow;           // segment limit (bits 0-15)
    uint16_t BaseLow;                   // base (bits 0-15)
    uint8_t BaseMiddle;                 // base (bits 16-23)
    uint8_t Access;                     // access flags
    uint8_t FlagsLimitHi;               // segment limit (bits 16-19) | flags (bits 20-23)
    uint8_t BaseHigh;                   // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t Limit;                     // sizeof(gdt)-1
    GDTEntry* Address;                  // pointer to table itself
} __attribute__((packed)) GDTDescriptor;

typedef enum {                          // access flags of GDT entry
    // ignoring bit 0 segment accessed flag
    GDT_ACCESS_CODE_READABLE                = 0x02, // 0: segment is execute only; 1: segment is read and execute
    GDT_ACCESS_DATA_WRITABLE                = 0x02, // 0: segment is read only; 1: segment is read and write
    GDT_ACCESS_CODE_CONFORMING              = 0x04, // 0: segment can be executed only from DPL; 1: segment can be executed from DPL or lower

    GDT_ACCESS_DATA_DIRECTION_NORMAL        = 0x00, // 0: segment grows down; 1: segment grows up
    GDT_ACCESS_DATA_DIRECTION_DOWN          = 0x04,

    // combining bits 3 and 4 results in these 3 conditions
    GDT_ACCESS_DESCRIPTOR_TSS               = 0x00, // 00: task state segment
    GDT_ACCESS_DATA_SEGMENT                 = 0x10, // 10: data segment
    GDT_ACCESS_CODE_SEGMENT                 = 0x18, // 11: code segment

    // bits 5 and 6 indicate descriptor privilege level (ring), 4 possible levels
    GDT_ACCES_RING0                         = 0x00, // 5 = 0, 6 = 0: kernel level
    GDT_ACCES_RING1                         = 0x20, // 5 = 1, 6 = 0
    GDT_ACCES_RING2                         = 0x40, // 5 = 0, 6 = 1
    GDT_ACCES_RING3                         = 0x60, // 5 = 1, 6 = 1: userland

    GDT_ACCESS_PRESENT                      = 0x80  // 0: descriptor is not present; 1: descriptor is present
} GDT_ACCESS;

typedef enum {
    // bits 0-3 are for the segment limit
    // ignoring bit 4 available for use by system software
    GDT_FLAG_64BIT                          = 0x20, // only used in 64-bit protected mode, 0: segment is not 64-bit; 1: segment is 64-bit, D/B must be 0
    GDT_FLAG_32BIT                          = 0x40, // 0: 16-bit segment; 1: 32-bit segment
    GDT_FLAG_16BIT                          = 0x00,
    GDT_FLAG_GRANULARITY_1B                 = 0x00, // 0: limit specified in bytes; 1: limit specified in 4kB blocks
    GDT_FLAG_GRANULARITY_4KB                = 0x80
} GDT_FLAG;

// helper macros
#define GDT_LIMIT_LOW(limit)                (limit & 0xFFFF)                            // gets lower 16 bits of limit
#define GDT_BASE_LOW(base)                  (base & 0xFFFF)                             // gets lower 16 bits of base
#define GDT_BASE_MIDDLE(base)               ((base >> 16) & 0xFF)                       // gets the middle bits of base
#define GDT_FLAGS_LIMIT_HI(limit, flags)    (((limit >> 16) & 0x0F) | (flags & 0xF0))   // gets high bits of limit and flag bits
#define GDT_BASE_HI(base)                   ((base >> 24) & 0xFF)                       // gets high bits of base

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit), \
    GDT_BASE_LOW(base), \
    GDT_BASE_MIDDLE(base), \
    access, \
    GDT_FLAGS_LIMIT_HI(limit, flags), \
    GDT_BASE_HI(base) \
}

GDTEntry g_GDT[] = {
    // NULL descriptor, 0 in all fields
    GDT_ENTRY(0, 0, 0, 0),

    // kernel 32-bit code segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCES_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4KB),

    // kernel 32-bit data segment
    GDT_ENTRY(0, 0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCES_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4KB)
};

GDTDescriptor g_GDTDescriptor = { sizeof(g_GDT) - 1, g_GDT };

void ASMCALL i686_GDT_load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void i686_GDT_Initialize() {
    i686_GDT_load(&g_GDTDescriptor, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
}