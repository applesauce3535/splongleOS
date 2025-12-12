#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "asm_wrappers.h"
#include "memory.h"
#include "stdio.h"

#define MAX_TASKS 16
#define KSTACK_SIZE 4096

struct thread_control_block;

typedef struct thread_control_block {
    /*
    other values that could be added
    the scheduling policy of the task
    the scheduling priority of the task
    the ID of the process the task/thread belongs to
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

extern thread_control_block* current_task_TCB;
static uint32_t g_num_tasks = 0;

void Multitasking_Init();
void ASMCALL switch_to_task(thread_control_block* next_thread);
thread_control_block* create_kernel_task(void (*entrypoint)(void));
void kill_kernel_task(thread_control_block* task);
void Schedule();
void yield();
void task_trampoline();