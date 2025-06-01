org 0x7C00  ; BIOS loads boot sector to 0x7C00
bits 16     ; tells assembler to emit 16 bit code

; macro for new line chara in hex
%define ENDL 0x0D, 0x0A  

;
; FAT12 header
;
jmp short start
nop

; original equipment manufacturer. kinda meaningless but set to this for max compatibility
bdb_oem:                    db 'MSWIN4.1'   ; 8 bytes
; indicates bytes per sector. 512 bytes for 1.4MB floppy
bdb_bytes_per_sector:       dw 512
; indicate sectors per cluster
bdb_sectors_per_cluster:    db 1
; indicates number of reserved sectors, just 1 for boot sector
bdb_reserved_sectors:       dw 1
; FAT's on storage, often 2
bdb_fat_count:              db 2
; number of directory entries
bdb_dir_entries_count:      dw 0E0h 
; total sectors, 512 bytes * 2880 bytes = 1.4MB
bdb_total_sectors:          dw 2880
; media descriptor, F0 indicates 3.5" floppy disc
bdb_media_descriptor_type:  db 0F0h
; sectors per FAT
bdb_sectors_per_fat:        dw 9
; sectors per track
bdb_sectors_per_track:      dw 18
; heads/side on storage media
bdb_head:                   dw 2
; hidden sectors
bdb_hidden_sectors:         dd 0
; large sectors
bdb_large_sectors:          dd 0

; extended boot sector
; drive number, 0 for floppy disc
ebr_drive_number:           db 0
                            db 0 ; just a reserved byte set to 0
; signature
ebr_signature:              db 29h ; must be 29 or 28 in hex
; serial number
ebr_volume_id:              db 23h ; value doesn't matter, so of course it's 35
; volume label, 11 bytes padded with spaces
ebr_volume_label:           db 'splongleOS '
; system id, 8 bytes padded with spaces
ebr_system_id:              db 'FAT12   '



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

    ; read something from floppy disk
    ; BIOS should set dl to drive number
    mov [ebr_drive_number], dl

    mov ax, 1           ; LBA=1, second sector of disk
    mov cl, 1           ; 1 sector to read
    mov bx, 0x7E00      ; data starts after bootloader
    call disk_read

    mov si, msg_hello   ; set si to start of the string
    call puts
    cli                 ; disable interrupts so CPU can't exit halt state
    hlt

;
; error handlers
;

floppy_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h             ; wait for keypress
    jmp 0FFFFh:0        ; jump to beginning of BIOS

.halt:
    cli                 ; disable interrupts so CPU can't exit halt state
    hlt

;
; disk routines
;

;
; converts a logical block addressing (LBA) address to a cylinder head sector (CHS) address
; parameters:
;   - ax: LBA address
; returns:
;   - cx [bits 0-5]: sector number
;   - cx [bits 6-15]: cylinder
;   - dh: head
;   this is exactly how the BIOS expects this to be stored
lba_to_chs:
    push ax                             ; save ax
    push dx                             ; save dl
    xor dx, dx                          ; dx = 0
    div word [bdb_sectors_per_track]    ; ax = LBA / SectorsPerTrack
                                        ; dx = LBA % SectorsPerTrack
    inc dx                              ; dx = (LBA % SectorsPerTrack + 1) = sector
    mov cx, dx                          ; cx = sector

    xor dx, dx
    div word [bdb_head]                 ; ax = (LBA / SectorsPerTrack) / heads = cylinder
                                        ; dx = (LBA / SectorsPerTrack) % heads = head
    mov dh, dl                          ; dh = head, dl is lower 8 bits of dx
    mov ch, al                          ; ch = cylinder (lower 8 bits), fills bits 8-15
    shl ah, 6
    or cl, ah                           ; fils bits 6-7 in cl for cylinder, cl bits 5-0 already contains sector
    pop ax  
    mov dl, al                          ; restore dl
    pop ax                              ; restore ax
    ret

;
; reads sectors from a disk
; parameters:
;   - ax: LBA address
;   - cl: number of sectors to read (up to 128)
;   - dl: drive number
;   - es:bx: memory address where to store read data
;
disk_read:
    push ax                             ; save registers we will modify
    push bx
    push cx
    push dx
    push di


    push cx                             ; save cl (number of sectors to read)
    call lba_to_chs
    pop ax                              ; al = number of sectors to read

    mov ah, 02h
    mov di, 3                           ; retry count
.retry:
    pusha                               ; save all registers because we don't know what BIOS is gonna do
    stc                                 ; manually set carry flag, some BIOS don't do this
    int 13h                             ; carry flag cleared = success
    jnc .done                           ; jump !C

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry                          ; jump !Z, 3 attempts

.fail:
    ; all attempts failed
    jmp floppy_error
.done:
    popa

    pop di                             ; restore registers modified
    pop dx
    pop cx
    pop bx
    pop ax
    ret

;
; resets disk controller
; parameters:
;   - dl: drive number
disk_reset:
    pusha 
    mov ah, 0
    stc
    int 13h
    jc floppy_error
    popa
    ret

msg_hello: db 'Hello, world? Hello, bitch. This is splongleOS.', ENDL, 0
msg_read_failed: db 'Read fail: Failure to read from floppy disk.', ENDL, 0

; fill the rest of the boot sector with 0s up to 510 bytes
times 510-($-$$) db 0   ; $ = memory offset of current line
                        ; $$ = memory offset of the beginning of the current section
                        ; so $ - $$ gives the whole size of the program measured in bytes

; boot signature
dw 0xAA55