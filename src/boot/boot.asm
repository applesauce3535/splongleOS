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
    ; setup data segments
    mov ax, 0   ; can't write to ds/es directly
    mov ds, ax  ; ds: points to string data to be copied to es
    mov es, ax  ; es: points to destination for string copy

    ; setup stack
    mov ss, ax  ; stack segment starts at 0
    mov sp, 0x7C00  ; stack grows downwards starting at address 0x7C00
                    ; stack starts at the beginning of the OS, because it would overwrite the OS if it started at the end

    ; some BIOS might start at 07C0:0000 instead of 0000:7C00, the next 3 lines make sure it's in
    ; the right location
    push es
    push word .after
    retf

.after:
    ; read something from floppy disk
    ; BIOS should set dl to drive number
    mov [ebr_drive_number], dl

    ; show loading message
    mov si, msg_loading   ; set si to start of the string
    call puts

    ; read drive parameters (sectors per track and head count)
    ; instead of relying on data on formatted disk
    push es
    mov ah, 08h
    int 13h
    jc floppy_error
    pop es
    and cl, 0x3F                        ; remove top 2 bits
    xor ch, ch                          ; clear ch
    mov [bdb_sectors_per_track], cx     ; sector count
    inc dh
    mov [bdb_head], dh                  ; head count

    ; compute LBA of root directory = reserved + FATs * sectors per FAT
    mov ax, [bdb_sectors_per_fat]      
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx                              ; ax = FATs * sectors per FAT
    add ax, [bdb_reserved_sectors]      ; ax = LBA of root
    push ax

    ; compute size of root = (32 * number of entries) / bytes per sector
    mov ax, [bdb_sectors_per_fat]
    shl ax, 5                           ; ax *= 32
    xor dx, dx                          ; clear dx of garbage
    div word [bdb_bytes_per_sector]     ; number of sectors we need to read

    test dx, dx                         ; if dx != 0, add 1
    jz .root_dir_after
    inc ax                              ; division remainder != 0, add 1
                                        ; means we have a sector only partially filled with entries

.root_dir_after:
    ; read root
    mov cl, al                          ; cl = number of sectors to read = size of root
    pop ax                              ; ax = lba of root
    mov dl, [ebr_drive_number]          ; dl = drive number (saved previously)
    mov bx, buffer                      ; es:bx = buffer (where its gonna be read to)
    call disk_read

    ; search for kernel.bin
    xor bx, bx                          ; bx = how many entries checked
    mov di, buffer                      ; di = current dir entry

; beginning of loop
.search_kernel:
    mov si, file_kernel_bin             ; kernel file we want
    mov cx, 11                          ; file name length is 11
    push di
    ; easy instruction for comparing two strings
    ; repe: repeat while equal
    ; cmpsb: compare two bytes in memory to eachother
    repe cmpsb 
    pop di
    je .found_kernel                    ; signifies kernel was found
    add di, 32                          ; if kernel wasn't found, add 32 to di (size of dir entry)
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_kernel
    jmp kernel_not_found_error

.found_kernel:
    ; di should have the address to the entry
    mov ax, [di + 26]                   ; first logical cluster field (offset 26)
    mov [kernel_cluster], ax
    ; load FAT from disk
    mov ax, [bdb_reserved_sectors]
    mov bx, buffer
    mov cl, [bdb_sectors_per_fat]
    mov dl, [ebr_drive_number]
    call disk_read
    ; read kernel and process FAT chain
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET

.load_kernel_loop:
    ; read next cluster
    mov ax, [kernel_cluster]
    ; hardcoded value specifically for 1.44MB floppy, needs to be changed for other storage mediums
    add ax, 31                          ; first cluster = (kernel cluster - 2) * sectors per cluster + start sector

    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read
    add bx, [bdb_bytes_per_sector]
    ; compute loc of next sector
    mov ax, [kernel_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx                              ; ax = index of entry in FAT, dx = cluster mod 2
    mov si, buffer
    add si, ax
    mov ax, [ds:si]                     ; read entry from FAT at index ax
    or dx, dx
    jz .even

.odd:
    shr ax, 4                           ; bit shift for upper 12 bits
    jmp .next_cluster_after

.even:
    and ax, 0x0FFF                      ; bit mask for lower 12 bits

.next_cluster_after:
    cmp ax, 0x0FF8                      ; end of FAT chain
    jae .read_finish                    ; jump if above or equal to end of chain

    mov [kernel_cluster], ax
    jmp .load_kernel_loop

.read_finish:
    ; jump to kernel
    mov dl, [ebr_drive_number]          ; boot device in dl

    mov ax, KERNEL_LOAD_SEGMENT         ; set segment registers
    mov ds, ax
    mov es, ax
    
    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

    jmp wait_key_and_reboot             ; hopefully shouldn't happen


    cli                                 ; disable interrupts so CPU can't exit halt state
    hlt

;
; error handlers
;

kernel_not_found_error:
    mov si, msg_kernel_not_found
    call puts
    jmp wait_key_and_reboot

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

msg_loading: db 'Loading...', ENDL, 0
msg_read_failed: db 'Disk read error.', ENDL, 0
file_kernel_bin: db 'KERNEL  BIN'
msg_kernel_not_found: db 'Kernel not found.', ENDL, 0
kernel_cluster: dw 0

KERNEL_LOAD_SEGMENT equ 0x2000
KERNEL_LOAD_OFFSET equ 0


; fill the rest of the boot sector with 0s up to 510 bytes
times 510-($-$$) db 0   ; $ = memory offset of current line
                        ; $$ = memory offset of the beginning of the current section
                        ; so $ - $$ gives the whole size of the program measured in bytes

; boot signature
dw 0xAA55

buffer: