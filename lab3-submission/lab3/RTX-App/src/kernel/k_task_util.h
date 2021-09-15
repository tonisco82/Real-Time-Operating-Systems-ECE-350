#ifndef K_TASK_UTIL_H_
#define K_TASK_UTIL_H_
#include "k_inc.h"
#include "k_mem.h"
#include "uart_polling.h"
#include "printf.h"

#define NUM_PRIORITIES 5 //0 (NULL task), 1-4 (LOWEST-HIGH)

// This is the TCB_queue which holds TCB's in ready queue structure.
typedef struct TCB_queue
{
	TCB *HEAD;
	TCB *TAIL;
} TCB_queue;

// This is the (GLOBAL) TCB_queue array which holds the TCB's in various priorities.
extern TCB_queue ready_TCB_queues[NUM_PRIORITIES];
extern TCB_queue dormant_TCB_queue;

// Linked List Function Declarations
// ----------------------------------------------------------------------------

void enqueue_back(struct TCB_queue *q, struct tcb *task);
void enqueue_front(struct TCB_queue *q, struct tcb *task);
void init_queues(void);
int prio_to_array_index(U8 prio);
void remove_tcb_given_tid(struct TCB_queue *q, task_t tid);
TCB *queue_pop(struct TCB_queue *q);
void init_queue(TCB_queue *tcb_q);

//ready queue array visualization
void visualize(void);

// -----------------------------------------------------------------------------

#endif
