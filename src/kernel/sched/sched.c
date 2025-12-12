#include "include/sched.h"

thread_control_block tasks[MAX_TASKS];
uint8_t used[MAX_TASKS];        // used slots in tasks array
uint8_t kernel_stacks[MAX_TASKS][KSTACK_SIZE];
static uint32_t g_task_id = 0;

thread_control_block* current_task_TCB = NULL;

// this function is going to put the kernel main function as task 0
void Multitasking_Init() {
    // set up used tasks
    for (uint8_t i = 0; i < MAX_TASKS; ++i) used[i] = 0;
    // mark slot 0 as used
    used[0] = 1;

    g_num_tasks = 1;
    current_task_TCB = &tasks[0];

    current_task_TCB->esp = (void*)i686_ReadESP();
    // this gets the PDE
    current_task_TCB->cr3 = (void*)i686_ReadCR3();
    // this value doesn't matter yet because i dont have userspace
    // it is supposed to be the top of the kernel stack for traps
    current_task_TCB->esp0 = (void*)i686_ReadESP();
    current_task_TCB->task_id = g_task_id++;
    current_task_TCB->next = current_task_TCB; // temporary, points to self
}

// this function will be called upon each task creation and deletion
void rebuild_task_list() {
    thread_control_block* prev = NULL;
    thread_control_block* first = NULL;

    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (used[i]) {
            thread_control_block* t = &tasks[i];
            // if we haven't made a first task yet, do it here
            if (!first) first = t;

            // now each time we find a subsequent task, we'll say the previous one points to the one we just found
            if (prev) prev->next = t;

            prev = t;
        }
    }

    // close the round robin loop if at least one task exists
    if (prev) prev->next = first;
}

thread_control_block* create_kernel_task(void (*entrypoint)(void)) {
    if (g_num_tasks >= MAX_TASKS) {
        printk("Error making new task: no space for new tasks left\n");
        return NULL; // no space left
    }

    // slot in a new task
    uint8_t free_slot = 0xFF;
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        // find first free task slot
        if (!used[i]) {
            used[i] = 1;
            free_slot = i;
            break;
        }
    }

    if (free_slot == 0xFF) return NULL;

    // now make a new task in our tasks array
    thread_control_block* t = &tasks[free_slot];

    // give it a stack
    uint8_t* stack = kernel_stacks[free_slot];

    // clear data in the new task to be safe
    memset(t, 0, sizeof(*t));

    /* 
    stack grows downwards so start at the end
    what we are doing is finding out what stack was being used by 
    this task, and then adding the size of a stack to that address
    */
    uint32_t* sp = (uint32_t*)(stack + KSTACK_SIZE);

    // push onto stack in the reverse order switch_to_task pops registers
    *(--sp) = (uint32_t)task_trampoline;    // return address goes to trampoline
    *(--sp) = 0;                            // EBX
    *(--sp) = 0;                            // ESI
    *(--sp) = 0;                            // EDI
    *(--sp) = 0;                            // EBP

    // after all of that, the stack pointer is just gonna be whatever sp is
    t->esp = sp;
    // get the PDE
    t->cr3 = (void*)i686_ReadCR3();
    t->esp0 = sp; // top of kernel stack, used for userspace
    t->task_id = g_task_id++;
    t->entrypoint = entrypoint;

    g_num_tasks++;

    rebuild_task_list();

    return t;
}

void kill_kernel_task(thread_control_block* task) {
    if (!task) return;
    int8_t task_index = -1;
    // scan tasks array for task that needs to be killed
    for (uint8_t i = 0; i < MAX_TASKS; ++i) {
        if (&tasks[i] == task) {
            task_index = i;
            break;
        }
    }
    if (task_index == -1) return;

    bool killing_current = (task == current_task_TCB);

    uint32_t killed_task = task->task_id;
    // mark the slot as unused
    used[task_index] = 0;
    // clear the tasks memory
    memset(task, 0, sizeof(thread_control_block));
    g_num_tasks--;
    // no tasks left is a fatal error
    if (g_num_tasks == 0) {
        printk("KERNEL PANIC: NO TASKS RUNNING");
        i686_panic();
    }
    printk("Killed task %d ", killed_task);

    if (killing_current) {
        // make sure we have a valid TCB after rebuild
        if (!current_task_TCB) {
            for (uint8_t i = 0; i < MAX_TASKS; ++i) {
                if (used[i]) {
                    current_task_TCB = &tasks[i];
                    break;
                }
            }
        }
        switch_to_task(current_task_TCB->next);
        while(1);
    }

    rebuild_task_list();
}

void Schedule() {
    switch_to_task(current_task_TCB->next);
}

void yield() {
    switch_to_task(current_task_TCB->next);
}

/* 
called as the 'entry' for every created task.
it calls the real entrypoint stored in the TCB.
when that function returns, we kill the task safely. 
*/
void task_trampoline() {
    // uint32_t esp;
    // __asm__ __volatile__("mov %%esp, %0" : "=r"(esp));
    // printk("trampoline: esp=%p\n", (void*)esp);
    // fetch the current task pointer
    thread_control_block* t = current_task_TCB;
    if (t && t->entrypoint) t->entrypoint();
    

    // if the entry returned, remove the task and schedule away
    kill_kernel_task(t);

    // if kill_kernel_task returns for some reason, switch to next task
    if (current_task_TCB && current_task_TCB->next)
        switch_to_task(current_task_TCB->next);

    // should never get here
    for (;;);
}