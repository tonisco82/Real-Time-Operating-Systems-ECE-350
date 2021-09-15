#ifndef K_TASK_UTIL_H_
#define K_TASK_UTIL_H_
#include "k_inc.h"
#include "k_mem.h"
#include "uart_polling.h"
#include "printf.h"

typedef struct RT_state {
	struct timeval period;
	U32 next_deadline;
	U32 jobs_done;
} RT_state;

extern RT_state g_rt_states[MAX_TASKS];

#define NUM_PRIORITIES 6 //0 realtime, 1 (NULL task), 2-5 (LOWEST-HIGH), 

// This is the TCB_queue which holds TCB's in ready queue structure.
typedef struct TCB_queue
{
	TCB *HEAD;
	TCB *TAIL;
} TCB_queue;

// This is the (GLOBAL) TCB_queue array which holds the TCB's in various priorities.
extern TCB_queue ready_TCB_queues[NUM_PRIORITIES];
extern TCB_queue dormant_TCB_queue;
extern TCB_queue suspended_TCB_queue;

// Linked List Function Declarations
// ----------------------------------------------------------------------------

void enqueue_back(struct TCB_queue *q, struct tcb *task);
void enqueue_front(struct TCB_queue *q, struct tcb *task);
void enqueue_sorted(struct TCB_queue *q, struct tcb *task, U32 (*f)(struct tcb *task));
void init_queues(void);
int prio_to_array_index(U8 prio);
void remove_tcb_given_tid(struct TCB_queue *q, task_t tid);
TCB *queue_pop(struct TCB_queue *q);
void init_queue(TCB_queue *tcb_q);
U32 timeval_to_ticks(TIMEVAL *p_tv);
//Get next deadline of rt task
U32 nd_val(struct tcb *task);
//Get current deadline of rt task
U32 cd_val(struct tcb *task);

//ready queue array visualization
void visualize(void);

// -----------------------------------------------------------------------------

#endif
