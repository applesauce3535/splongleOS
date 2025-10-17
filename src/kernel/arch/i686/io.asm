; these are just a wrapper for reading and writing to ports

global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global i686_panic
i686_panic:
    cli
    hlt

global crash_me
crash_me:
    mov eax, 0xFFFFFFFF
    mov edx, 0xFFFFFFFF
    mov ebx, 2
    div ebx
    ret