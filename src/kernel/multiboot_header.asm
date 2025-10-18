[bits 32]
; Multiboot v1 header for GRUB

SECTION .multiboot
align 4
; magic, flags, checksum
dd 0x1BADB002   ; multiboot header identifier magic number
dd 0x00000000   ; GRUB flags, don't use any for now
dd -(0x1BADB002 + 0x00000000)   ; must make the sum of all 3 fields equal 0

; Ensure the header is in the output so linkers don't discard it
global multiboot_header
multiboot_header:
    ; empty label, header data above is what matters
    nop
