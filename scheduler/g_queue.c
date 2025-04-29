#include "g_queue.h"

#include <stdbool.h>

void queue_init(struct queue_t *queue)
{
    queue->head = NULL; // Initialize head to NULL
    queue->tail = NULL; // Initialize tail to NULL
}

int queue_is_empty(struct queue_t *queue)
{
    return queue->head == NULL;
}

void queue_push(struct queue_t *queue, int thread_idx)
{
    struct queue_item* thread = malloc(sizeof(struct queue_item)); // Allocate memory for the new thread
    thread->thread_idx = thread_idx; // Set thread index
    if (queue_is_empty(queue))
    {
        queue->head = thread; // Set head to the new thread
        queue->tail = thread; // Set tail to the new thread
    }
    else
    {
        queue->tail->next = thread; // Link the new thread to the end of the queue
        queue->tail = thread;       // Update tail to the new thread
    }
    thread->next = NULL; // Set next of the new thread to NULL
}

int queue_pop(struct queue_t *queue)
{
    if (queue_is_empty(queue))
        return -1; // If the queue is empty, return NULL

    struct queue_item* thread = queue->head; // Get the thread at the head of the queue
    int thread_idx = thread->thread_idx; // Store the thread index
    queue->head = thread->next; // Move head to the next thread
    if (queue_is_empty(queue))
        queue->tail = NULL; // If the queue is now empty, set tail to NULL
    
    free(thread); // Free the memory of the popped thread

    return thread_idx; // Return the popped thread
}

