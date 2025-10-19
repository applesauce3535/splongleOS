[bits 32]
; Multiboot v1 header for GRUB

SECTION .multiboot
align 4
; magic, flags, checksum
dd 0x1BADB002   ; multiboot header identifier magic number
dd 0x00000003   ; GRUB flags, aligning all boot modules and passing mem info to kernel
dd -(0x1BADB002 + 0x00000003)   ; must make the sum of all 3 fields equal 0

; Ensure the header is in the output so linkers don't discard it
global multiboot_header
multiboot_header:
    ; empty label, header data above is what matters
    nop

; assembly wrapper — forwards multiboot registers to C entry
; expects: EAX = multiboot magic, EBX = pointer to multiboot_info_t
global start
extern kernel_main

SECTION .text
align 4
start:
    ; preserve registers? we call into C which will save as needed
    push ebp
    mov ebp, esp

    ; push parameters in C order (right-to-left for cdecl)
    push ebx            ; push mbinfo pointer
    push eax            ; push magic
    call kernel_main

    ; return from kernel_main -> infinite loop
    cli
.hang:
    hlt
    jmp .hang
