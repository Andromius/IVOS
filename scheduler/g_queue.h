#include <stdlib.h>

struct queue_item {
    int thread_idx; // pointer to the thread
    struct queue_item* next; // pointer to the next item in the queue
};

struct queue_t {
    struct queue_item* head; // pointer to the first process in the queue
    struct queue_item* tail; // pointer to the last process in the queue
};

void queue_init(struct queue_t* queue);
int queue_is_empty(struct queue_t* queue);
void queue_push(struct queue_t* queue, int thread_idx);
int queue_pop(struct queue_t* queue);
