[bits 32]
; Multiboot v1 header for GRUB

SECTION .multiboot
align 4
; magic, flags, checksum
dd 0x1BADB002   ; multiboot header identifier magic number
dd 0x00000003   ; 0x1 | 0x2 => align modules, memory info (do NOT request drive info here)
dd -(0x1BADB002 + 0x00000003)   ; must make the sum of all 3 fields equal 0
dd 0, 0, 0, 0, 0    ; using ELF format, the values will all be 0
; next values related to graphics
dd 0            ; linear graphic mode
dd 800          ; screen width
dd 600          ; screen height
dd 32           ; depth



; Ensure the header is in the output so linkers don't discard it
global multiboot_header
multiboot_header:
    ; empty label, header data above is what matters
    nop

SECTION .bss
align 16
stack_bottom:
    resb 16384 * 8
stack_top:

SECTION .boot

; assembly wrapper — forwards multiboot registers to C entry
; expects: EAX = multiboot magic, EBX = pointer to multiboot_info_t
global _start
extern kernel_main

_start:
    ; preserve registers? we call into C which will save as needed
    push ebp
    mov ebp, esp
    push eax            ; push magic number
    mov eax, (initial_page_dir - 0xC0000000)
    ; now we're gonna setup paging
    mov cr3, eax        ; tells CPU where location of page directory and page table is for each process
    mov ecx, cr4
    or  ecx, 0x10
    mov cr4, ecx        ; turning on physical address extension
    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx        ; enable paging
    pop eax             ; regain magic

    jmp higher_half

SECTION .text
higher_half:
    mov esp, stack_top

    ; push parameters in C order (right-to-left for cdecl)
    push ebx            ; push mbinfo pointer
    push eax            ; push magic
    xor ebp, ebp        ; reset stack base
    call kernel_main

    ; return from kernel_main -> infinite loop
    cli
.hang:
    hlt
    jmp .hang

; setting up 1024 entrys
SECTION .data
align 4096
global initial_page_dir
initial_page_dir:
    dd 10000011b
    times 768-1 dd 0
    dd (0 << 22) | 10000011b
    dd (1 << 22) | 10000011b
    dd (2 << 22) | 10000011b
    dd (3 << 22) | 10000011b
    times 256-4 dd 0