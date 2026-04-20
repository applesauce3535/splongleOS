#include "include/sched.h"

void kernel_idle_task();

bool task_switches_postponed = false;
uint32_t postpone_task_switches_counter = 0;    // this is how many locks the kernel is holding


int IRQ_disable_counter = 0;                    // this is how many pieces of code want
                                                // IRQs disabled

thread_control_block tasks[MAX_TASKS];
uint8_t used[MAX_TASKS];        // used slots in tasks array
uint8_t kernel_stacks[MAX_TASKS][KSTACK_SIZE];
static uint32_t g_task_id = 0;

thread_control_block* current_task_TCB = NULL;
thread_control_block* g_first_ready = NULL;
thread_control_block* g_last_ready = NULL;
thread_control_block* idle_task = NULL;
task_state_segment g_tss = {0};

// this function is going to put the kernel main function as task 0
void Multitasking_Init() {
    // set up used tasks
    for (uint8_t i = 0; i < MAX_TASKS; ++i) used[i] = 0;
    // mark slot 0 as used
    used[0] = 1;
    current_task_TCB = &tasks[0];
    g_num_tasks = 1;
    current_task_TCB->state = RUNNING;

    current_task_TCB->esp = (void*)i686_ReadESP();
    // this gets the PDE
    current_task_TCB->cr3 = (void*)i686_ReadCR3();
    // this value doesn't matter yet because i dont have userspace
    // it is supposed to be the top of the kernel stack for traps
    current_task_TCB->esp0 = (void*)i686_ReadESP();
    current_task_TCB->task_id = g_task_id++;
    current_task_TCB->next = NULL;
    // the ready to run list is initially empty
    g_first_ready = g_last_ready = NULL;
    // this task will never be terminated
    idle_task = create_kernel_task(kernel_idle_task);
}

// create new kernel tasks, returns a pointer to a TCB for the task
thread_control_block* create_kernel_task(void (*entrypoint)(void)) {
    if (g_num_tasks >= MAX_TASKS) {
        printk("Oops, sorry about that.: no space for new tasks left\n");
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
    *(--sp) = (uint32_t)entrypoint;         // return address (EIP)
    *(--sp) = 0;                            // EBX
    *(--sp) = 0;                            // ESI
    *(--sp) = 0;                            // EDI
    *(--sp) = 0;                            // EBP

    // after all of that, the stack pointer is just gonna be whatever sp is
    t->esp = sp;
    // get the PDE
    t->cr3 = (void*)i686_ReadCR3();
    t->esp0 = stack + KSTACK_SIZE; // top of kernel stack, used for userspace
    t->task_id = g_task_id++;
    t->entrypoint = entrypoint;
    t->state = READY_TO_RUN;
    t->next = NULL;

    // add new task to ready to run list
    if (g_last_ready) {
        g_last_ready->next = t;
        g_last_ready = t;
    }
    // case where this is the first ready to run task created
    else g_first_ready = g_last_ready = t;
    g_num_tasks++;

    return t;
}


// switches to the next ready to run task
// WARNING: the caller is responsible for making sure the scheduler is locked
// before calling
void Schedule() {
    // if locks are held, set postpone flag and do nothing
    if (postpone_task_switches_counter > 0) {
        task_switches_postponed = true;
        return;
    }
    if (g_first_ready != NULL) {
        thread_control_block* task = g_first_ready;
        g_first_ready = task->next;
        if (task == idle_task) {
            // try finding an alternative so idle task doesn't have to run
            if (g_first_ready != NULL) {
                // idle task was selected but others are ready
                task = g_first_ready;   // this will make the idle task be skipped
                idle_task->next = task->next;
                g_first_ready = idle_task;
            }
            else if (current_task_TCB->state == RUNNING) {
                // no other tasks, but current was running, so let it remain running
                return;
            }
            // at this point there's no other options and the idle task has to run
        }
        switch_to_task(task);
    }
}


// disables IRQs at the start of critical sections and keeps track of how many
// pices of code want IRQs disabled via IRQ_disable_counter
void lock_scheduler() {
#ifndef SMP
    i686_DisableInts();
    IRQ_disable_counter++;
#endif
}

// decrements IRQ_disable_counter and, if it equals 0, enables IRQs
void unlock_scheduler() {
#ifndef SMP
    IRQ_disable_counter--;
    if (IRQ_disable_counter == 0) i686_EnableInts();
#endif
}

// blocks a task for a specified reason and then schedule's the next ready to run task
void block_task(int reason) {
    lock_scheduler();
    current_task_TCB->state = reason;
    Schedule();
    unlock_scheduler();
}

void unblock_task(thread_control_block* task) {
    lock_scheduler();
    // only 1 task was running, or the idle task was running, so pre-empt this previously blocked task
    if (g_first_ready == NULL || (current_task_TCB == idle_task)) switch_to_task(task);
    // there is at least 1 other ready to run task, so don't pre-empt
    else {
        g_last_ready->next = task;
        // put the blocked task at the back of list
        g_last_ready = task;
    }
    unlock_scheduler();
}

void acquire_lock() {
#ifndef SMP
    i686_DisableInts();
    IRQ_disable_counter++;
    postpone_task_switches_counter++;
#endif
}

void release_lock() {
#ifndef SMP
    postpone_task_switches_counter--;
    // if no locks are held and the postpone flag is set, unset it and start scheduling
    if (postpone_task_switches_counter == 0) {
        if (task_switches_postponed) {
            task_switches_postponed = false;
            Schedule();
        }
    }
    IRQ_disable_counter--;
    if (IRQ_disable_counter == 0) i686_EnableInts();
#endif
}

void kernel_idle_task() {
    for(;;) {
        i686_Halt();
    }
}
