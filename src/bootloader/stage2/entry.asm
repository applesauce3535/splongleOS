bits 16 ; tell assembler we're in 16-bit mode

section .entry ; tells nasm to put code in the entry section

extern __bss_start
extern __end

extern start ; entry point from C
global entry

entry:
    cli             ; 1) disable interrupts

    ; save boot drive
    mov [g_BootDrive], dl

    ; setting up a basic stack for small memory model
    ; data segment and stack segment are the same in this model
    ; they have been set up by stage one, so just copy them to eachother
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0
    mov bp, sp      ; set stack and base pointers to 0

    ; switch to protected
    call EnableA20  ; 2) enable A20 gate for backwards compatibility
    call LoadGDT    ; 3) set up global descriptor table

    ; 4) set protection enable flag in control register 0
    mov eax, cr0    ; read value of cr0
    or al, 1        ; set protected mode bit to 1
    mov cr0, eax    ; put back into cr0

    ; 5) far jump into protected mode
    jmp dword 08h:.pmode    ; second entry in GDT, offset 8, first entry is 0 so yeah


.pmode:
    ; now in 32 bit protected mode
    [bits 32]
    ; skipping steps 6-8
    ; 9) setup segment registers
    mov ax, 0x10    ; 3rd entry of GDT, offset of 16 bits
    mov ds, ax
    mov ss, ax

    ; clear bss (uninitialized data)
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi    ; gets size of bss section
    mov al, 0
    cld             ; clear direction flag so it goes forward
    rep stosb       ; stosb takes whatever is in al and stores it into the memory pointed to by edi
                    ; rep goes until ecx is 0

    ; expect boot drive in dl, send it as arg to cstart
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start

    cli
    hlt

EnableA20:
    [bits 16]
    ; disable keyboard
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    ; read control output port 
    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    ; write control output port
    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al

    call A20WaitInput
    pop eax
    or al, 2    ; bit 2 = A20 bit
    out KbdControllerDataPort, al

    ; enable keyboard
    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput
    ret

A20WaitInput:
    [bits 16]
    ; wait until status bit 2 (input buffer) is 0
    ; by reading from command port, we read status byte
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    [bits 16]
    ; wait until status bit 1 (output buffer) is 1 so it can be read
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret

LoadGDT:
    [bits 16]
    lgdt [g_GDTDesc]    ; special CPU instruction to load the GDT, only take descriptor as input
    ret




; constants
KbdControllerDataPort               equ 0x60    ; send/receive data from keyboard controller
KbdControllerCommandPort            equ 0x64    ; read/write status/command register
KbdControllerDisableKeyboard        equ 0xAD    ; disable keyboard
KbdControllerEnableKeyboard         equ 0xAE    ; enable keyboard
KbdControllerReadCtrlOutputPort     equ 0xD0    ; read controller output port
KbdControllerWriteCtrlOutputPort    equ 0xD1    ; write controller output port

ScreenBuffer                        equ 0xB8000

g_GDT:
    ; NULL descriptor
    dq 0        ; first entry must always be 0
    ; 32 bit code segment
    dw 0FFFFh   ; limit (bits 0-15) = 0xFFFFF for full 32 bit range
    dw 0        ; base (bits 0-15) = 0x0
    db 0        ; base (bits 16-23)
    db 10011010b; access flags (present, ring 00, code segment, executable, direction 0, readable, accessed)
    db 11001111b; granularity (4kB), 32 bit protected, reserved 00, limit (bits 16-19)
    db 0        ; base high

    ; 32 bit data segment (identical to code, but executable set to 0)
    dw 0FFFFh   ; limit (bits 0-15) = 0xFFFFF for full 32 bit range
    dw 0        ; base (bits 0-15) = 0x0
    db 0        ; base (bits 16-23)
    db 10010010b; access flags (present, ring 00, data segment, executable 0, direction 0, writable, accessed)
    db 11001111b; granularity (4kB), 32 bit protected, reserved 00, limit (bits 16-19)
    db 0        ; base high

    ; 16 bit code segment
    dw 0FFFFh   ; limit (bits 0-15) = 0xFFFFF
    dw 0        ; base (bits 0-15) = 0x0
    db 0        ; base (bits 16-23)
    db 10011010b; access flags (present, ring 00, code segment, executable, direction 0, readable, accessed)
    db 00001111b; granularity (1B), 16 bit protected, reserved 00, limit (bits 16-19)
    db 0        ; base high

    ; 16 bit data segment (identical to code, but executable set to 0)
    dw 0FFFFh   ; limit (bits 0-15) = 0xFFFFF
    dw 0        ; base (bits 0-15) = 0x0
    db 0        ; base (bits 16-23)
    db 10010010b; access flags (present, ring 00, data segment, executable 0, direction 0, writable, accessed)
    db 00001111b; granularity (1B), 16 bit protected, reserved 00, limit (bits 16-19)
    db 0        ; base high

; needed to load the GDT
g_GDTDesc:
    dw g_GDTDesc - g_GDT - 1        ; limit = size of GDT
    dd g_GDT                        ; address of GDT

g_BootDrive:
    db 0
