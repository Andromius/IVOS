#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <asm-generic/signal-defs.h>
#include <stdint.h>
#include <stdbool.h>

#include "g_semaphore.h"
#include "gthr.h"
#include "gthr_struct.h"

void sem_init(struct semaphore_t *sem, int initial_value)
{
    sem->value = initial_value; // Initialize semaphore value
    sem->queue = malloc(sizeof(struct queue_t)); // Allocate memory for the queue
    if (sem->queue == NULL)
    {
        perror("Failed to allocate memory for semaphore queue");
        exit(EXIT_FAILURE);
    }
    queue_init(sem->queue);     // Initialize the queue for waiting threads
}

void sem_wait(struct semaphore_t *sem)
{
    sigset_t old_mask, new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigaddset(&new_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    if (sem->value > 0)
    {
        sem->value--; // Decrement semaphore value
    }
    else
    {
        queue_push(sem->queue, gt_current - gt_table); // Add current thread to the queue
        //printf("%d\n", gt_current - gt_table);
        update_thread_state(gt_current, Blocked); // Update thread state to Blocked
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

void sem_post(struct semaphore_t *sem)
{
    sigset_t old_mask, new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigaddset(&new_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    if (queue_is_empty(sem->queue))
    {
        sem->value++; // Increment semaphore value
    }
    else
    {
        int thread_idx = queue_pop(sem->queue); // Remove a thread from the queue
        struct gt *thread = &gt_table[thread_idx]; // Get the thread from the table
        update_thread_state(thread, Ready); // Update thread state to Ready
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

void sem_destroy(struct semaphore_t *sem)
{
    while(queue_is_empty(sem->queue))
    {
        int thread_idx = queue_pop(sem->queue);
        struct gt *thread = &gt_table[thread_idx]; // Get the thread from the table
        update_thread_state(thread, Ready); // Update thread state to Ready
    }

    free(sem->queue);
}

void sem_try_wait(struct semaphore_t *sem)
{
    sigset_t old_mask, new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigaddset(&new_mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    if (sem->value > 0)
    {
        sem->value--; // Decrement semaphore value
    }
    else
    {
        perror("Semaphore is not available\n");
    }
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}
