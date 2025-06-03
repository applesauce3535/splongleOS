org 0x0  
bits 16     ; tells assembler to emit 16 bit code

; macro for new line chara in hex
%define ENDL 0x0D, 0x0A  

start:
    mov si, msg_hello   ; set si to start of the string
    call puts

.halt:
    cli
    hlt
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

msg_hello: db 'Hello, world? Hello, bitch. This is splongleOS, and this is the kernel.', ENDL, 0