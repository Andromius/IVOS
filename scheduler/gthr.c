#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

#include <asm-generic/signal-defs.h>

#include "gthr.h"
#include "gthr_struct.h"

struct gt gt_table[MaxGThreads];                                                // statically allocated table for thread control
struct gt *gt_current;                                                          // pointer to current thread
bool (*get_next_thread)(struct gt **);
int total_tickets = 0;

// function triggered periodically by timer (SIGALRM)
void gt_alarm_handle(int sig)
{
    gt_schedule();
}

// function triggered by CTRL+C (SIGINT)
void gt_int_handle(int sig)
{
    gt_print_stats();
}

// Time-related helper functions
void timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    if (result->tv_usec < 0)
    {
        result->tv_sec--;
        result->tv_usec += 1000000;
    }
}

void timeval_add(struct timeval *result, struct timeval *x, struct timeval *y)
{
    result->tv_sec = x->tv_sec + y->tv_sec;
    result->tv_usec = x->tv_usec + y->tv_usec;
    if (result->tv_usec >= 1000000)
    {
        result->tv_sec++;
        result->tv_usec -= 1000000;
    }
}

double timeval_to_microseconds(struct timeval *tv)
{
    return tv->tv_sec * 1000000.0 + tv->tv_usec;
}

void init_timeval(struct timeval *tv)
{
    tv->tv_sec = 0;
    tv->tv_usec = 0;
}

int timeval_is_zero(struct timeval *tv)
{
    return (tv->tv_sec == 0 && tv->tv_usec == 0);
}

void update_running_stats(struct timeval *new_time, struct timeval *min_time,
                          struct timeval *max_time, double *avg, double *var,
                          unsigned long count)
{
    double new_value = timeval_to_microseconds(new_time);

    if (timeval_is_zero(min_time) ||
        (new_time->tv_sec < min_time->tv_sec ||
         (new_time->tv_sec == min_time->tv_sec && new_time->tv_usec < min_time->tv_usec)))
    {
        memcpy(min_time, new_time, sizeof(struct timeval));
    }

    if (timeval_is_zero(max_time) ||
        (new_time->tv_sec > max_time->tv_sec ||
         (new_time->tv_sec == max_time->tv_sec && new_time->tv_usec > max_time->tv_usec)))
    {
        memcpy(max_time, new_time, sizeof(struct timeval));
    }

    // Update average and variance using Welford's online algorithm
    double delta = new_value - *avg;
    *avg += delta / count;
    double delta2 = new_value - *avg;
    *var += delta * delta2;
}

// Print thread statistics
void gt_print_stats(void)
{
    struct gt *p;

    printf("\n\033[1;36m====================================================================================\033[0m\n");
    printf("\033[1;36m                                Thread Statistics                                      \033[0m\n");
    printf("\033[1;36m====================================================================================\033[0m\n");

    // Print scheduling algorithm
    printf("\033[1;35mScheduling Algorithm: %s\033[0m\n", 
           get_next_thread == round_robin ? "Round Robin" : 
           get_next_thread == round_robin_prio ? "Round Robin with Priorities" :
           "Lottery");
    
    // Run Time Statistics Table
    printf("\033[1;36m====================================================================================\033[0m\n");
    printf("\033[1;36m                               Run Time Statistics                                    \033[0m\n");
    if (get_next_thread == round_robin_prio) {
        printf("\033[1;37mID | State   | Run Count | Priority |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    } else if (get_next_thread == lottery) {
        printf("\033[1;37mID | State   | Run Count | Tickets  |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    } else {
        printf("\033[1;37mID | State   | Run Count |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    }
    printf("\033[90m====================================================================================\033[0m\n");

    for (p = &gt_table[0]; p < &gt_table[MaxGThreads]; p++)
    {
        if (p->state == Unused && p->run_count == 0)
            continue;

        const char *state_color;
        const char *state_str;
        switch (p->state)
        {
        case Running:
            state_color = "\033[1;32m";
            state_str = "Running";
            break;
        case Ready:
            state_color = "\033[1;33m";
            state_str = "Ready";
            break;
        case Unused:
            state_color = "\033[1;31m";
            state_str = "Unused";
            break;
        case Blocked:
            state_color = "\033[1;31m";
            state_str = "Blocked";
            break;
        default:
            state_color = "\033[1;37m";
            state_str = "Unknown";
            break;
        }

        double std_dev_run = 0.0;
        if (p->run_count > 1)
        {
            std_dev_run = sqrt(p->var_run_time_us / (p->run_count - 1));
        }

        double total_run_ms = timeval_to_microseconds(&p->total_running) / 1000.0;

        if (get_next_thread == round_robin_prio) {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%9lu\033[0m | \033[1;34m%8d\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->run_count,
                   p->priority,
                   total_run_ms,
                   p->avg_run_time_us,
                   timeval_to_microseconds(&p->min_run_time),
                   timeval_to_microseconds(&p->max_run_time),
                   std_dev_run);
        } else if (get_next_thread == lottery) {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%9lu\033[0m | \033[1;34m%3d-%-4d\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->run_count,
                   p->tickets_lo, p->tickets_hi,
                   total_run_ms,
                   p->avg_run_time_us,
                   timeval_to_microseconds(&p->min_run_time),
                   timeval_to_microseconds(&p->max_run_time),
                   std_dev_run);
        } else {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%9lu\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->run_count,
                   total_run_ms,
                   p->avg_run_time_us,
                   timeval_to_microseconds(&p->min_run_time),
                   timeval_to_microseconds(&p->max_run_time),
                   std_dev_run);
        }
    }
    printf("\033[90m====================================================================================\033[0m\n");

    // Wait Time Statistics Table
    printf("\n\033[1;36m====================================================================================\033[0m\n");
    printf("\033[1;36m                               Wait Time Statistics                                   \033[0m\n");
    if (get_next_thread == round_robin_prio) {
        printf("\033[1;37mID | State   | Wait Count | Priority |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    } else if (get_next_thread == lottery) {
        printf("\033[1;37mID | State   | Wait Count | Tickets  |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    } else {
        printf("\033[1;37mID | State   | Wait Count |  Total (ms)  |  Avg (μs)  |  Min (μs)  |  Max (μs)  | Std Dev   \033[0m\n");
    }
    printf("\033[90m====================================================================================\033[0m\n");

    for (p = &gt_table[0]; p < &gt_table[MaxGThreads]; p++)
    {
        if (p->state == Unused && p->wait_count == 0)
            continue;

        const char *state_color;
        const char *state_str;
        switch (p->state)
        {
        case Running:
            state_color = "\033[1;32m";
            state_str = "Running";
            break;
        case Ready:
            state_color = "\033[1;33m";
            state_str = "Ready";
            break;
        case Unused:
            state_color = "\033[1;31m";
            state_str = "Unused";
            break;
        case Blocked:
            state_color = "\033[1;31m";
            state_str = "Blocked";
            break;
        default:
            state_color = "\033[1;37m";
            state_str = "Unknown";
            break;
        }

        double std_dev_wait = 0.0;
        if (p->wait_count > 1)
        {
            std_dev_wait = sqrt(p->var_wait_time_us / (p->wait_count - 1));
        }

        double total_wait_ms = timeval_to_microseconds(&p->total_waiting) / 1000.0;

        if (get_next_thread == round_robin_prio) {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%10lu\033[0m | \033[1;34m%8d\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->wait_count,
                   p->priority,
                   total_wait_ms,
                   p->avg_wait_time_us,
                   timeval_to_microseconds(&p->min_wait_time),
                   timeval_to_microseconds(&p->max_wait_time),
                   std_dev_wait);
        } else if (get_next_thread == lottery) {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%10lu\033[0m | \033[1;34m%3d-%-4d\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->wait_count,
                   p->tickets_lo, p->tickets_hi,
                   total_wait_ms,
                   p->avg_wait_time_us,
                   timeval_to_microseconds(&p->min_wait_time),
                   timeval_to_microseconds(&p->max_wait_time),
                   std_dev_wait);
        } else {
            printf("\033[1;36m%2ld\033[0m | %s%-7s\033[0m | \033[1;33m%10lu\033[0m | \033[1;36m%11.2f\033[0m | \033[1;37m%10.2f\033[0m | \033[1;32m%10.2f\033[0m | \033[1;31m%10.2f\033[0m | \033[1;35m%9.2f\033[0m\n",
                   (long)(p - gt_table),
                   state_color, state_str,
                   p->wait_count,
                   total_wait_ms,
                   p->avg_wait_time_us,
                   timeval_to_microseconds(&p->min_wait_time),
                   timeval_to_microseconds(&p->max_wait_time),
                   std_dev_wait);
        }
    }
    printf("\033[90m====================================================================================\033[0m\n");
    printf("\033[1;36m====================================================================================\033[0m\n");
}

// Initialize thread statistics fields
void init_thread_stats(struct gt *thread)
{
    gettimeofday(&thread->creation_time, NULL);
    gettimeofday(&thread->last_state_change, NULL);

    init_timeval(&thread->total_running);
    init_timeval(&thread->total_waiting);

    thread->run_count = 0;
    init_timeval(&thread->min_run_time);
    init_timeval(&thread->max_run_time);
    init_timeval(&thread->last_run_time);
    thread->avg_run_time_us = 0.0;
    thread->var_run_time_us = 0.0;

    thread->wait_count = 0;
    init_timeval(&thread->min_wait_time);
    init_timeval(&thread->max_wait_time);
    init_timeval(&thread->last_wait_time);
    thread->avg_wait_time_us = 0.0;
    thread->var_wait_time_us = 0.0;
}

// initialize first thread as current context
void gt_init(void)
{
    srand(time(NULL));
    gt_current = &gt_table[0];          // initialize current thread with thread #0
    gt_current->state = Running;        // set current to running
    gt_current->priority = MaxPriority; // set priority to 0
    gt_current->tickets_lo = MaxPriority;
    gt_current->tickets_hi = MinPriority;
    gt_current->skip_count = 0;         // set skip count to 0
    total_tickets += MinPriority + 1;
    init_thread_stats(gt_current);

    signal(SIGALRM, gt_alarm_handle); // register SIGALRM, signal from timer generated by alarm
    signal(SIGINT, gt_int_handle);    // register SIGINT handler for Ctrl+C
}

// exit thread
void __attribute__((noreturn)) gt_return(int ret)
{
    if (gt_current != &gt_table[0])
    {                                             // if not an initial thread,
        gt_current->state = Unused;               // set current thread as unused
        free((void *)(gt_current->ctx.rsp + 16)); // free the stack
        gt_schedule();                            // yield and make possible to switch to another thread
        assert(!"reachable");                     // this code should never be reachable ... (if yes, returning function on stack was corrupted)
    }
    while (gt_schedule()); // if initial thread, wait for other to terminate
    exit(ret);
}

void update_thread_state(struct gt *thread, int new_state)
{
    struct timeval now;

    sigset_t old_mask, new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &new_mask, &old_mask);

    gettimeofday(&now, NULL);

    if (thread->state != new_state)
    {
        struct timeval elapsed;
        timeval_subtract(&elapsed, &now, &thread->last_state_change);

        if (thread->state == Running)
        {
            timeval_add(&thread->total_running, &thread->total_running, &elapsed);

            thread->run_count++;
            memcpy(&thread->last_run_time, &elapsed, sizeof(struct timeval));
            update_running_stats(&elapsed, &thread->min_run_time, &thread->max_run_time,
                                 &thread->avg_run_time_us, &thread->var_run_time_us, thread->run_count);
        }
        else if (thread->state == Ready || thread->state == Blocked)
        {
            timeval_add(&thread->total_waiting, &thread->total_waiting, &elapsed);

            thread->wait_count++;
            memcpy(&thread->last_wait_time, &elapsed, sizeof(struct timeval));
            update_running_stats(&elapsed, &thread->min_wait_time, &thread->max_wait_time,
                                 &thread->avg_wait_time_us, &thread->var_wait_time_us, thread->wait_count);
        }

        thread->state = new_state;
        memcpy(&thread->last_state_change, &now, sizeof(struct timeval));
    }

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

bool round_robin(struct gt **thread)
{
    while ((*thread)->state != Ready)
    {// iterate through gt_table[] until we find new thread in state Ready
        if (++(*thread) == &gt_table[MaxGThreads]) // at the end rotate to the beginning
            *thread = &gt_table[0];

        if ((*thread)->state == Blocked)
            continue;

        if ((*thread) == gt_current)
            return true;
    }
    return true;
}

bool round_robin_prio(struct gt **thread)
{ // set current thread as first one to check
    while (true)
    {
        if (++(*thread) == &gt_table[MaxGThreads]) // at the end rotate to the beginning
            *thread = &gt_table[0];
        
        if ((*thread)->state == Blocked)
            continue;

        if (((*thread)->state == Ready) && ((*thread)->priority - (*thread)->skip_count <= MaxPriority))
        {
            (*thread)->skip_count = 0;
            return true;
        }
        else
        {
            (*thread)->skip_count++;
        }

        if ((*thread) == gt_current) // did not find any other Ready threads
            return true;
    }
    return false;
}

bool lottery(struct gt** thread) 
{
    int winning_ticket = rand() % total_tickets;
    for (struct gt* p = &gt_table[0]; p < &gt_table[MaxGThreads]; p++)
    {
        if (p->state == Blocked)
            continue; // skip unused threads

        if ((p->state == Ready || p->state == Running) &&
            winning_ticket >= p->tickets_lo && winning_ticket <= p->tickets_hi)
        {
            *thread = p;
            return true;
        }
    }

    return round_robin(thread);
}

void set_scheduling_algorithm(bool (*algorithm)(struct gt **))
{
    get_next_thread = algorithm;
}

// switch from one thread to other
bool gt_schedule(void)
{
    struct gt *p;
    struct gt_context *old, *new;

    gt_reset_sig(SIGALRM);
    p = gt_current;
    if (!get_next_thread(&p))
    {
        return false;
    }

    if (gt_current->state != Unused && gt_current->state != Blocked)
    { // switch current to Ready and new thread found in previous loop to Running
        update_thread_state(gt_current, Ready);
    }

    update_thread_state(p, Running);

    old = &gt_current->ctx; // prepare pointers to context of current (will become old)
    new = &p->ctx;          // and new to new thread found in previous loop
    gt_current = p;         // switch current indicator to new thread
    gt_switch(old, new);    // perform context switch (assembly in gtswtch.S)

    return true;
}

// return function for terminating thread
void gt_stop(void)
{
    gt_return(0);
}

// create new thread by providing pointer to function that will act like "run" method
int gt_create(void (*f)(void), int priority)
{
    char *stack;
    struct gt *p;

    for (p = &gt_table[0];; p++)         // find an empty slot
        if (p == &gt_table[MaxGThreads]) // if we have reached the end, gt_table is full and we cannot create a new thread
            return -1;
        else if (p->state == Unused)
            break; // new slot was found

    stack = malloc(StackSize); // allocate memory for stack of newly created thread
    if (!stack)
        return -1;

    *(uint64_t *)&stack[StackSize - 8] = (uint64_t)gt_stop; //  put into the stack returning function gt_stop in case function calls return
    *(uint64_t *)&stack[StackSize - 16] = (uint64_t)f;      //  put provided function as a main "run" function
    p->ctx.rsp = (uint64_t)&stack[StackSize - 16];          //  set stack pointer

    if (priority < MaxPriority)
        priority = MaxPriority; //  set minimum priority
    else if (priority > MinPriority)
        priority = MinPriority; //  set maximum priority

    p->priority = priority; //  set priority
    p->skip_count = 0;      //  set skip count to 0
    p->tickets_lo = total_tickets;
    p->tickets_hi = total_tickets + (MinPriority - p->priority);
    total_tickets += MinPriority - p->priority + 1;
    // Initialize thread statistics
    init_thread_stats(p);

    // Set thread state to Ready
    update_thread_state(p, Ready);

    return 0;
}

// resets SIGALRM signal
void gt_reset_sig(int sig)
{
    if (sig == SIGALRM)
    {
        alarm(0); // Clear pending alarms if any
    }

    sigset_t set;                         // Create signal set
    sigemptyset(&set);                    // Clear set
    sigaddset(&set, sig);                 // Set signal (we use SIGALRM)
    sigprocmask(SIG_UNBLOCK, &set, NULL); // Fetch and change the signal mask

    if (sig == SIGALRM)
    {
        ualarm(500, 500); // Schedule signal after given number of microseconds
    }
}

// uninterruptible sleep
int gt_uninterruptible_nanosleep(time_t sec, long nanosec)
{
    struct timespec req;
    req.tv_sec = sec;
    req.tv_nsec = nanosec;

    do
    {
        if (0 != nanosleep(&req, &req))
        {
            if (errno != EINTR)
                return -1;
        }
        else
        {
            break;
        }
    } while (req.tv_sec > 0 || req.tv_nsec > 0);
    return 0;
}
