[bits 32]

extern i686_ISR_Handler

; CPU pushes to the stack: ss, esp, eflags, cs, eip

%macro ISR_NOERRORCODE 1

global i686_ISR%1:
i686_ISR%1:
    push 0              ; always push a dummy variable for interrupts that don't also push an error code
    push %1             ; push interrupt number
    jmp isr_common

%endmacro

%macro ISR_ERRORCODE 1

global i686_ISR%1
i686_ISR%1:
    push %1
    jmp isr_common

%endmacro

%include "arch/i686/isrs_gen.inc"

isr_common:
    pusha               ; push all general purpose regs
    xor eax, eax
    mov ax, ds
    push eax
    
    mov ax, 0x10        ; use kernel's data segment, this is not automatically changed by the processor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pass pointer to stack to C, so we can access all pushed info
    call i686_ISR_Handler
    add esp, 4

    pop eax             ; remove arg from stack
    ; restore all modified segment regs
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; restore general purpose registers
    add esp, 8          ; remove error code and interrupt number
    iret                ; will pop: eip, cs, eflags, esp, ss