#include "g_queue.h"

struct semaphore_t {
    int value; // semafor
    struct queue_t* queue; // fronta procesu čekajících na semafor
};

void sem_init(struct semaphore_t* sem, int initial_value);
void sem_wait(struct semaphore_t* sem); // "P" operace
void sem_post(struct semaphore_t* sem); // "V" operace
void sem_destroy(struct semaphore_t* sem);
void sem_try_wait(struct semaphore_t* sem); // "P" operace bez blokování
