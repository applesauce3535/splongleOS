; these are just a wrapper for reading and writing to ports

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
    cli
    mov eax, [esp + 4]
    invlpg [eax]
    sti
    ret

global i686_ChangePD
i686_ChangePD:
    cli
    mov eax, [esp + 4]
    mov cr3, eax
    sti
    ret

global i686_EnablePaging
i686_EnablePaging:
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    ret

global i686_ReadCR2
i686_ReadCR2:
    mov eax, cr2
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