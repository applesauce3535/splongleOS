org 0x7C00  ; BIOS loads boot sector to 0x7C00
bits 16     ; tells assembler to emit 16 bit code

; macro for new line chara in hex
%define ENDL 0x0D, 0x0A  

start:
    jmp main    ; sets main as the entry point to the program

;
; prints a string to the screen
; params:
;   - ds:si points to string
;

puts:
    ; save the registers modified to the stack
    push si
    push ax

; AH = 0x0E     Teletype output
; AL = character to print
; BH = page number (usually 0)
; BL = text attribute (only for color modes)

.loop:
    lodsb       ; loads a byte from ds:si into al and then increments si
    or al, al   ; check if next character is null
    jz .done    ; jump-zero: jump to done label if previous instruction set Z flag
    mov ah, 0x0E    ; beginning of using video interrupt from BIOS to display text
    mov bh, 0   ; sets page number to 0
    int 0x10    ; interrupt code needed to display output
    jmp .loop   ; jump to loop label if Z flag was not set

.done:
    pop ax
    pop si 
    ret 

main:
    ; setup data segments
    mov ax, 0   ; can't write to ds/es directly
    mov ds, ax  ; ds: points to string data to be copied to es
    mov es, ax  ; es: points to destination for string copy

    ; setup stack
    mov ss, ax  ; stack segment starts at 0
    mov sp, 0x7C00  ; stack grows downwards starting at address 0x7C00
                    ; stack starts at the beginning of the OS, because it would overwrite the OS if it started at the end

    mov si, msg_hello   ; set si to start of the string
    call puts

    hlt

.halt:
    jmp .halt

msg_hello: db 'Hello, world? Hello, bitch. This is splongleOS.', ENDL, 0

; fill the rest of the boot sector with 0s up to 510 bytes
times 510-($-$$) db 0   ; $ = memory offset of current line
                        ; $$ = memory offset of the beginning of the current section
                        ; so $ - $$ gives the whole size of the program measured in bytes

; boot signature
dw 0xAA55