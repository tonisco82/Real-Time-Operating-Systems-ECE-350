#include "k_task_util.h"

// This is the (GLOBAL) TCB_queue which holds the TCB's in various priorities.

TCB_queue ready_TCB_queues[NUM_PRIORITIES];

// Store the dormant TCBs
TCB_queue dormant_TCB_queue;

// The suspended TCB for 
TCB_queue suspended_TCB_queue;

RT_state g_rt_states[MAX_TASKS];  // state of real time tasks 

// Linked List Function Definitions
// ----------------------------------------------------------------------------

void enqueue_back(struct TCB_queue *q, struct tcb *task)
{
	// If queue is empty, then new node is both HEAD and TAIL.
	if (q->HEAD == NULL && q->TAIL == NULL)
	{
		q->HEAD = task;
		q->TAIL = task;
		q->HEAD->prev = NULL;
		q->TAIL->next = NULL;
	}
	else
	{
		// Add the new node at the end of queue and change old TAIL to point to it.
		q->TAIL->next = task;
		task->prev = q->TAIL;
		q->TAIL = task;
		q->TAIL->next = NULL;
	}
	return;
}

void enqueue_front(struct TCB_queue *q, struct tcb *task)
{
	// If queue is empty, then new node is both HEAD and TAIL.
	if (q->HEAD == NULL && q->TAIL == NULL)
	{
		q->HEAD = task;
		q->TAIL = task;
		q->HEAD->prev = NULL;
		q->TAIL->next = NULL;
	}
	else
	{
		// Add the new node at the start of queue and change old HEAD to point to it.
		q->HEAD->prev = task;
		task->next = q->HEAD;
		q->HEAD = task;
		q->HEAD->prev = NULL;
	}

	return;
}


U32 nd_val(struct tcb *task) {
	return g_rt_states[task->tid].next_deadline;
}


U32 cd_val(struct tcb *task) {
	U32 p = timeval_to_ticks(&g_rt_states[task->tid].period);
	return g_rt_states[task->tid].next_deadline - p;
}


// val is the value to use for the sorting
// this is going be an ascending queue
void enqueue_sorted(struct TCB_queue *q, struct tcb *task, U32 (*f)(struct tcb *task)) {
	U32 val = f(task);

	struct tcb *node = q->HEAD;

	if (node == NULL || val < f(node)) {
		enqueue_front(q, task);
	} else {
		while (node != NULL && val > f(node)) {
			node = node->next;
		}

		if (node == NULL) {
			enqueue_back(q, task);
		} else {
			struct tcb *ins_node = node->prev;
			ins_node->next = task;
			task->prev = ins_node;
			task->next = node;
			node->prev = task;
		}
	}

	return;
}

U32 timeval_to_ticks(TIMEVAL *p_tv) {

	// 1E6 / 500 = 2000
	return (p_tv->sec * 2000) + (p_tv->usec / 500);
}

void init_queues(void)
{
	for (int i = 0; i < NUM_PRIORITIES; i++)
	{
		init_queue(&ready_TCB_queues[i]);
	}

	init_queue(&dormant_TCB_queue);
	init_queue(&suspended_TCB_queue);
}

//initialize HEAD and TAIL to NULL
void init_queue(TCB_queue *tcb_q)
{
	tcb_q->HEAD = NULL;
	tcb_q->TAIL = NULL;
}

int prio_to_array_index(U8 prio)
{
	if (prio == PRIO_RT) {
		// 6 - 1 = 5
		return NUM_PRIORITIES - 1;
	}

	if (prio == PRIO_NULL)
	{
		return 0;
	}
	else
	{
		// HIGHEST IS 0x80 | LOWEST IS 0x83
		// 0x84 - prio
		return LOWEST + 1 - prio;
	}
}

void remove_tcb_given_tid(struct TCB_queue *q, task_t tid)
{
	TCB *tcb = &g_tcbs[tid];
	int isHead = q->HEAD->tid == tid;
	int isTail = q->TAIL->tid == tid;

	if (isHead)
	{
		q->HEAD = tcb->next;
		if (q->HEAD)
		{
			q->HEAD->prev = NULL;
		}
	}

	if (isTail)
	{
		// This TCB is the tail
		q->TAIL = tcb->prev;
		if (q->TAIL)
		{
			q->TAIL->next = NULL;
		}
	}

	if (!isHead && !isTail)
	{
		// TCB is in the middle
		TCB *prev = tcb->prev;
		TCB *next = tcb->next;

		prev->next = next;
		next->prev = prev;
	}
}

TCB *queue_pop(struct TCB_queue *q)
{
	if (q->HEAD == NULL)
	{
		return NULL;
	}

	TCB *popped = q->HEAD;
	remove_tcb_given_tid(q, popped->tid);
	return popped;
}

void visualize(void)
{

	TCB *traverser = NULL;

	for (int i = 4; i >= 0; i--)
	{
		traverser = ready_TCB_queues[i].HEAD;
		printf("PRIO=%d: HEAD => ", i);
		while (traverser != NULL)
		{
			printf("[(tid=%d)", traverser->tid);
			printf("(prio=%d)", prio_to_array_index(traverser->prio));
			printf("(priv=%d)", traverser->priv);
			printf("(state=%d)] => ", traverser->state);
			traverser = traverser->next;
		}
		printf("TAIL\n");
	}
}

// -----------------------------------------------------------------------------
