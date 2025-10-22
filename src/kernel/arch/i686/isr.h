#pragma once
#include <stdint.h>

#define ASMCALL __attribute__((cdecl))

typedef struct {
    // in the reverse order they are pushed
    uint32_t ds;                                            // data segment push
    uint32_t edi, esi, ebp, kernel_esp, ebx, edx, ecx, eax; // pusha
    uint32_t interrupt, error_code;                         // we push interrupt, error is pushed by CPU
    uint32_t eip, cs, eflags, esp, ss;                      // automatically pushed by CPU
} __attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers* regs);

void i686_ISR_Initialize();
void i686_ISR_RegisterHandler(int interrupt, ISRHandler handler);
void dump_regs(Registers* regs);