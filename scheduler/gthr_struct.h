#ifndef GTHR_STRUCT_H
#define GTHR_STRUCT_H

extern struct gt gt_table[MaxGThreads];                                                // statically allocated table for thread control
extern struct gt *gt_current;                                                          // pointer to current thread
extern bool (*get_next_thread)(struct gt **);
extern int total_tickets;

#endif