#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "asm_wrappers.h"
#include "memory.h"
#include "stdio.h"

#define MAX_TASKS 16
#define KSTACK_SIZE 4096
#define RUNNING         0
#define READY_TO_RUN    1
#define PAUSED          2

struct thread_control_block;
struct task_state_segment;

typedef struct thread_control_block {
    /*
    other values that could be added
    the scheduling policy of the task
    the scheduling priority of the task
    a "task name" string
    the amount of CPU time the task has consumed so far
    */
    void* esp;                      // task's kernel stack top
    void* esp0;                     
    void* cr3;                      // task's virtual address space
    struct thread_control_block* next;     // linked list of tasks
    uint8_t state;                  // task state (ready, running, blocked)
    uint32_t task_id;
    void (*entrypoint)(void);
} thread_control_block; 

typedef struct task_state_segment {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} task_state_segment;

extern thread_control_block* current_task_TCB;
extern task_state_segment g_tss;
static uint32_t g_num_tasks = 0;

void Multitasking_Init();
void ASMCALL switch_to_task(thread_control_block* next_thread);
thread_control_block* create_kernel_task(void (*entrypoint)(void));
void Schedule();
void lock_scheduler();
void unlock_scheduler();
void block_task(int reason);
void unblock_task(thread_control_block* task);
void acquire_lock();
void release_lock();