bits 16 ; tell assembler we're in 16-bit mode

section _ENTRY class=CODE ; tells nasm to put code in the entry section

extern _cstart ; entry point from C
global entry

entry:
    cli
    ; setting up a basic stack for small memory model
    ; data segment and stack segment are the same in this model
    ; they have been set up by stage one, so just copy them to eachother
    mov ax, ds
    mov ss, ax
    mov sp, 0
    mov bp, sp      ; set stack and base pointers to 0
    sti             ; should be no interrupts when setting up the stack

    ; expect boot drive in dl, send it as arg to cstart
    xor dh, dh
    push dx
    call _cstart

    cli
    hlt             ; halt system if we return from the cstart func
