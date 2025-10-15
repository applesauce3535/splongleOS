[bits 32]
; Multiboot v1 header for GRUB
; NASM syntax

SECTION .multiboot
align 4
; magic, flags, checksum
dd 0x1BADB002
dd 0x00000003
dd -(0x1BADB002 + 0x00000003)

; Ensure the header is in the output so linkers don't discard it
global multiboot_header
multiboot_header:
    ; empty label — header data above is what matters
    nop
