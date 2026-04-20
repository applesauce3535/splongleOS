; credit to OSDev Wiki for their kernel multitasking tutorial.

; WARNING: the caller is responsible for making sure the scheduler is locked
; before calling

[bits 32]
extern current_task_TCB
extern g_first_ready
extern g_last_ready
extern g_tss
extern postpone_task_switches_counter
extern task_switches_postponed
global switch_to_task

; Offsets inside thread_control_block
%define TCB_ESP    0
%define TCB_ESP0   4
%define TCB_CR3    8
%define TCB_NEXT   12
%define TCB_STATE  16
;C declaration:
;   void switch_to_task(thread_control_block *next_thread);
;
;WARNING: Caller is expected to disable IRQs before calling, and enable IRQs again after function returns

switch_to_task:

    ; before anything check if switching is postponed
    cmp dword [postpone_task_switches_counter], 0
    je .continue
    mov dword [task_switches_postponed], 1
    ret

.continue:
    ;Save previous task's state

    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved
    push ebx
    push esi
    push edi
    push ebp

    ; save the previous task
    mov edi,[current_task_TCB]    ;edi = address of the previous task's "thread control block"
    mov [edi+TCB_ESP],esp         ;Save ESP for previous task's kernel stack in the thread's TCB

    ; check if previous task was running
    mov al, [edi+TCB_STATE]       ; al = state of previous task
    cmp al, 0                     ; RUNNING = 0, READY_TO_RUN = 1
    jne .skip_put_back            ; it was not running, don't put it back
    mov byte [edi+TCB_STATE], 1   ; it was running, mark it as ready to run instead
    ; append previous task to the ready list
    mov ebx, [g_last_ready]
    test ebx, ebx                 ; bitwise AND, if ebx is anything but 0 then last != first
    jz .first_ready               ; if we got 0, then the first and last ready to run are the same
    mov [ebx+TCB_NEXT], edi       ; g_last_ready->next = previous task
    mov [g_last_ready], edi
    jmp .ready_done

.first_ready:
    mov [g_first_ready], edi      ; g_first_ready = previous task (list was empty)
    mov [g_last_ready], edi       ; make them the same

.ready_done:
    mov dword [edi+TCB_NEXT], 0         ; since it's the end of the list, g_last_ready->next = NULL

.skip_put_back:
    ;Load next task's state

    mov esi,[esp+(4+1)*4]         ;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov [current_task_TCB],esi    ;Current task's TCB is the next task TCB
    mov byte [esi+TCB_STATE], 0   ; next task is running

    mov esp,[esi+TCB_ESP]         ;Load ESP for next task's kernel stack from the thread's TCB
    mov eax,[esi+TCB_CR3]         ;eax = address of page directory for next task
    mov ebx,[esi+TCB_ESP0]        ;ebx = address for the top of the next task's kernel stack
    mov [g_tss+4],ebx               ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes), this is at g_tss offset 4 bytes
    mov ecx,cr3                   ;ecx = previous task's virtual address space

    cmp eax,ecx                   ; Does the virtual address space need to be changed?
    je .doneVAS                   ; no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,eax                   ; yes, load the next task's virtual address space

.doneVAS:

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret                           ; Load next task's EIP from its kernel stack, this will be at the top of the stack
                                  ; when this function returns