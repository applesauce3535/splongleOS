#include "include/sched.h"

thread_control_block tasks[MAX_TASKS];
uint8_t used[MAX_TASKS];        // used slots in tasks array
uint8_t kernel_stacks[MAX_TASKS][KSTACK_SIZE];
uint32_t num_tasks = 0;

thread_control_block* current_task_TCB = NULL;

thread_control_block* get_current_task_TCB() {
    return current_task_TCB;
}

void Multitasking_Init() {
    num_tasks = 1;
    current_task_TCB = &tasks[0];

    current_task_TCB->esp = (void*)i686_ReadESP();
    current_task_TCB->cr3 = (void*)i686_ReadCR3();
    // this value doesn't matter yet because i dont have userspace
    current_task_TCB->esp0 = (void*)i686_ReadESP();
    current_task_TCB->next = current_task_TCB; // temporary
}


thread_control_block* create_kernel_task(void (*entrypoint)(void)) {
    if (num_tasks >= MAX_TASKS) return NULL; // no space left

    // slot in a task
    thread_control_block* t = &tasks[num_tasks];

    // give it a stack
    uint8_t* stack = kernel_stacks[num_tasks];

    memset(t, 0, sizeof(*t));

    // stack grows downwards so start at end
    uint32_t* sp = (uint32_t*)(stack + KSTACK_SIZE);

    *(--sp) = (uint32_t)entrypoint; // return address (EIP)
    *(--sp) = 0;                    // EBX
    *(--sp) = 0;                    // ESI
    *(--sp) = 0;                    // EDI
    *(--sp) = 0;                    // EBP

    t->esp = sp;
    t->cr3 = (void*)i686_ReadCR3();
    t->esp0 = sp; // top of kernel stack, used for userspace

    // add to task list using round robin
    thread_control_block* head = &tasks[0];
    thread_control_block* last = &tasks[num_tasks - 1];

    last->next = t;
    t->next = head;

    num_tasks++;

    return t;
}
