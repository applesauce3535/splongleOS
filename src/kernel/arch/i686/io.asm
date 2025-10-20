; simple assembly wrappers

global i686_outb
i686_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global i686_inb
i686_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global i686_EnableInts
i686_EnableInts:
    sti
    ret

global i686_DisableInts
i686_DisableInts:
    cli
    ret

global i686_InvalidatePage
i686_InvalidatePage:
    [bits 32]
    mov eax, [esp + 4]
    invlpg [eax]
    ret

global i686_GetPage
i686_GetPage:
    mov eax, cr3
    ret

global i686_ChangePage
i686_ChangePage:
    mov eax, [esp + 4]
    mov cr3, eax
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