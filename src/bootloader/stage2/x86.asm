; macros for switching between modes

%macro x86_EnterRealMode 0
    [bits 32]
    ; go back to real mode
    ; skipping step 1
    ; 2) jump to 16 bit protected mode segment
    jmp word 18h:.pmode16     ; 4th entry of GDT

.pmode16:
    [bits 16]
    ; skipping steps 3-5
    ; 6) clear protected mode bit from cr0
    mov eax, cr0    ; read value of cr0
    and al, ~1        ; set protected mode bit to 0
    mov cr0, eax    ; put back into cr0
    ; 7) jump to real mode
    jmp word 00h:.rmode

.rmode:
    ; 8) setup segments
    mov ax, 0
    mov ds, ax
    mov ss, ax
    ; 9) enable interrupts
    sti
%endmacro

%macro x86_EnterProtectedMode 0
    cli
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
%endmacro

%macro LinearToSegOffset 4
; convert linear address to segment:offset address
; args:
;   linear address
;   (out) target segment (e.g. es)
;   target 32-bit register to use (e.g. eax)
;   target lower 16-bit half of above (e.g. ax)
;   
    mov %3, %1      ; put linear address into register
    shr %3, 4       ; segment is in lower 4 bits of register
    mov %2, %4      ; put segment into register
    mov %3, %1      ; put linear address into register
    and %3, 0xF     ; remove other bits from address except last 4 (offset)

%endmacro

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

;
; bool _cdecl x86_Disk_getDriveParams(uint8_t drive, uint8_t* driveTypeOut, uint16_t* cylindersOut, uint16_t* sectorsOut, uint16_t* headsOut);
;
global x86_Disk_getDriveParams
x86_Disk_getDriveParams:
    [bits 32]
    ; make new call frame
    push ebp                ; save old call frame
    mov ebp, esp            ; initialize new call frame

    x86_EnterRealMode
    [bits 16]
    ; save regs
    push bx
    push esi
    push es
    push di

    ; call int 13h
    ; bp = call frame, bp + 4 = return addr, bp + 8 = first arg
    mov dl, [bp + 8]    ; dl = drive
    mov ah, 08h
    mov di, 0           ; es:di = 0000:0000, protects against BIOS bugs
    mov es, di
    stc
    int 13h

    ; out params
    mov eax, 1
    sbb eax, 0
    ; drive type from bl
    LinearToSegOffset [bp + 12], es, esi, si
    mov es:[si], bl

    ; cylinders
    mov bl, ch          ; cylinders lower bits in ch
    mov bh, cl          ; cylinders upper bits in cl (6-7)
    shr bh, 6
    inc bx              ; for some reason, it was subtracting 1 from the number of cylinders, this fixes it

    LinearToSegOffset [bp + 16], es, esi, si
    mov es:[si], bx

    ; sectors
    xor ch, ch          ; sectors = lower 5 bits in cl
    and cl, 3Fh

    LinearToSegOffset [bp + 20], es, esi, si
    mov es:[si], cx

    ; heads
    inc dh              ; another thing where it subtracted 1
    mov cl, dh          ; dh = heads
    LinearToSegOffset [bp + 24], es, esi, si
    mov es:[si], cx

    ; restore regs
    pop di
    pop es
    pop esi
    pop bx

    ; return
    
    push eax            ; eax gets changed when going into protected mode so push it

    x86_EnterProtectedMode
    [bits 32]

    pop eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

;
; bool _cdecl x86_Disk_Reset(uint8_t drive);
;
global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    x86_EnterRealMode
    [bits 16]
    mov ah, 0
    mov dl, [bp + 8]    ; dl = drive
    stc
    int 13h

    mov ax, 1
    sbb ax, 0           ; 1 = true, 0 = false
    push eax

    x86_EnterProtectedMode
    [bits 32]
    pop eax
    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

;
; bool _cdecl x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t sector, uint16_t head, uint8_t count, uint8_t far* dataOut);
;
global x86_Disk_Read
x86_Disk_Read:
    [bits 32]
    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp         ; initialize new call frame

    x86_EnterRealMode
    [bits 16]
    ; save registers
    push ebx
    push es

    mov dl, [bp + 8]    ; dl = drive
    mov ch, [bp + 12]    ; ch = lower 8 bits of cylinder
    mov cl, [bp + 13]    ; cl = cylinder bits 6-7
    shl cl, 6
    
    mov al, [bp + 16]
    and al, 3Fh
    or cl, al           ; cl = sector to bits 0-5

    mov dh, [bp + 20]    ; dh = head

    mov al, [bp + 24]   ; al = count

    LinearToSegOffset [bp + 28], es, ebx, bx    ; es:bx = pointer to data out

    mov ah, 02h
    stc
    int 13h

    ; return value
    mov eax, 1
    sbb eax, 0           ; 1 = true, 0 = false
 
    ; restore registers
    pop es
    pop ebx
    push eax

    x86_EnterProtectedMode
    [bits 32]
    pop eax
    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret
