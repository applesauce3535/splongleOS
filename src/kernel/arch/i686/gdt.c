#include "gdt.h"
#include <stdint.h>


typedef struct {
    uint16_t LimitLow;              // limit (bits 0-15)
    uint16_t BaseLow;               // base (bits 0-15)
    uint8_t BaseMiddle;             // base (bits 16-25)
    uint8_t Access;                 // access
    uint8_t FlagsLimitHigh;         // limit (bits 16-19) | flags
    uint8_t BaseHigh;               // base (bits 24-31)
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t Limit;                 // sizeof(gdt) - 1
    GDTEntry* GDTAddress;           // address of GDT
} __attribute__((packed)) GDTDescriptor;

// this is the access byte flags for the GDT
typedef enum {
    // we can ignore bit 0
    GDT_ACCESS_CODE_READABLE = 0x02,    // bit 1 will represent something different
                                        // depending on if it's a code or data segment
    GDT_ACCESS_DATA_WRITABLE = 0x02,

    GDT_ACCESS_CODE_CONFORMING = 0x04,  // next bit is similar to previous one
    GDT_ACCESS_DATA_DIRECTION_NORMAL = 0x00,
    GDT_ACCESS_DATA_DIRECTION_DOWN = 0x04,

    // bits 3 and 4 can be combined
    GDT_ACCESS_DATA_SEGMENT = 0x10,     // bits 3 and 4 indicate data segment
    GDT_ACCESS_CODE_SEGMENT = 0x18,     // bits 3 and 4 represent code segment
    GDT_ACCESS_DESCRIPTOR_TSS = 0x00,   // bits 3 and 4 represent a segment different from the previous 2

    // bits 5 and 6 represent privelege ring CPU needs to be in to access the segment
    // 0 is the highest and 3 is the lowest
    GDT_ACCESS_RING0 = 0x00,            // ring 0
    GDT_ACCESS_RING1 = 0x20,            // ring 1
    GDT_ACCESS_RING2 = 0x40,            // ring 2
    GDT_ACCESS_RING3 = 0x60,            // ring 3

    // bit 7 represents whether a segment is enabled (present) or disabled
    GDT_ACCESS_PRESENT = 0x80

} GDT_ACCESS;

// this is the other flags of the GDT
typedef enum {
    // can ignore bits 0-4
    // bit 5 indicates if it is a 64-bit segment
    GDT_FLAG_64BIT = 0x20,
    // bit 6 indicates whether 16 or 32-bit segment
    GDT_FLAG_32BIT = 0x40,
    GDT_FLAG_16BIT = 0x00,
    // bit 7 represents granularity (0 = specified in bytes, 1 = specified in 4kB blocks)
    GDT_FLAG_GRANULARITY_1B = 0x00,
    GDT_FLAG_GRANULARITY_4KB = 0x80,    
} GDT_FLAGS;

// helper macros
#define GDT_LIMIT_LOW(limit)        (limit & 0x0FFFF)   // helper for getting low bits of limit
#define GDT_BASE_LOW(base)          (base & 0x0FFFF)    // helper for getting low bits of base
#define GDT_BASE_MIDDLE(base)       ((base >> 16) & 0x0FF)  // helper for getting middle bits of base
#define GDT_FLAGS_LIMIT_HIGH(limit, flags)  (((limit >> 16) * 0x0F) | (flags & 0xF0))   // helper to get segment limit + flags
#define GDT_BASE_HIGH(base)         ((base >> 24) & 0x0FF) // helper to get high bits of base

#define GDT_ENTRY(base, limit, access, flags) { \
    GDT_LIMIT_LOW(limit),                       \
    GDT_BASE_LOW(base),                         \
    GDT_BASE_MIDDLE(base),                      \
    access,                                     \
    GDT_FLAGS_LIMIT_HIGH(limit, flags),         \
    GDT_BASE_HIGH(base)                         \
}

GDTEntry g_GDT[] = {
    // NULL descriptor
    GDT_ENTRY(0, 0, 0, 0),
    // kernel 32-bit code segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_CODE_SEGMENT | GDT_ACCESS_CODE_READABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4KB),
    // kernel 32-bit data segment
    GDT_ENTRY(0,
              0xFFFFF,
              GDT_ACCESS_PRESENT | GDT_ACCESS_RING0 | GDT_ACCESS_DATA_SEGMENT | GDT_ACCESS_DATA_WRITABLE,
              GDT_FLAG_32BIT | GDT_FLAG_GRANULARITY_4KB)
};

GDTDescriptor g_GDTDescriptor = {sizeof(g_GDT) - 1, g_GDT};

void __attribute__((cdecl)) i686_GDT_Load(GDTDescriptor* descriptor, uint16_t codeSegment, uint16_t dataSegment);

void i686_GDT_Initialize() {
    i686_GDT_Load(&g_GDTDescriptor, i686_GDT_CODE_SEGMENT, i686_GDT_DATA_SEGMENT);
}