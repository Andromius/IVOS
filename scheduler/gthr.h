#include <sys/time.h>

enum {
    MaxGThreads = 5,            // Maximum number of threads, used as array size for gttbl
    StackSize = 0x400000,       // Size of stack of each thread
    MinPriority = 10,          // Maximum priority
    MaxPriority = 0           // Minimum priority
};

struct gt {
    // Saved context, switched by gtswtch.S (see for detail)
    struct gt_context {
        uint64_t rsp;
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
        uint64_t rbp;
    } ctx;
    
    // Thread state
    enum e_state {
        Unused,
        Running,
        Ready,
        Blocked
    } state;
    
    // Statistics tracking
    struct timeval creation_time;     // When thread was created
    struct timeval total_running;     // Total time in Running state
    struct timeval total_waiting;     // Total time in Ready state
    struct timeval last_state_change; // Last time the thread state changed
    
    // Run-time statistics
    unsigned long run_count;          // Number of times this thread has run
    struct timeval min_run_time;      // Minimum time slice run
    struct timeval max_run_time;      // Maximum time slice run
    struct timeval last_run_time;     // Last run time slice
    double avg_run_time_us;           // Average run time in microseconds
    double var_run_time_us;           // Variance of run times
    
    // Wait-time statistics
    unsigned long wait_count;         // Number of times this thread has waited
    struct timeval min_wait_time;     // Minimum time slice waited
    struct timeval max_wait_time;     // Maximum time slice waited
    struct timeval last_wait_time;    // Last wait time slice
    double avg_wait_time_us;          // Average wait time in microseconds
    double var_wait_time_us;          // Variance of wait times

    int priority;               // Thread priority
    int skip_count;            // Number of times this thread has been skipped

    int tickets_lo;
    int tickets_hi;
};

void gt_init(void);                                                     // initialize gttbl
void gt_return(int ret);                                                // terminate thread
void gt_switch(struct gt_context * old, struct gt_context * new);       // declaration from gtswtch.S
bool gt_schedule(void);                                                 // yield and switch to another thread
void gt_stop(void);                                                     // terminate current thread
int gt_create(void( * f)(void), int priority);                                        // create new thread and set f as new "run" function
void gt_reset_sig(int sig);                                             // resets signal
void gt_alarm_handle(int sig);                                          // periodically triggered by alarm
void gt_int_handle(int sig);                                            // handler for SIGINT (Ctrl+C)
void gt_print_stats(void);                                              // print thread statistics
int gt_uninterruptible_nanosleep(time_t sec, long nanosec);             // uninterruptible sleep
bool round_robin(struct gt** thread);
bool round_robin_prio(struct gt** thread);                                         // round robin scheduler
bool lottery(struct gt** thread);
void set_scheduling_algorithm(bool (*algorithm)(struct gt**));
void update_thread_state(struct gt *thread, int new_state);


// Helper functions for time calculations
void timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y);
void timeval_add(struct timeval *result, struct timeval *x, struct timeval *y);
double timeval_to_microseconds(struct timeval *tv);
void init_timeval(struct timeval *tv);
int timeval_is_zero(struct timeval *tv);
void update_running_stats(struct timeval *new_time, struct timeval *min_time, 
                          struct timeval *max_time, double *avg, double *var, 
                          unsigned long count);
