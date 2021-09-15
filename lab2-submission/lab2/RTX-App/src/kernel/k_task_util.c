#include "k_task_util.h"

// This is the (GLOBAL) TCB_queue which holds the TCB's in various priorities.
TCB_queue ready_TCB_queues[NUM_PRIORITIES];

// Store the dormant TCBs
TCB_queue dormant_TCB_queue;

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

void init_queues(void)
{
    for (int i = 0; i < NUM_PRIORITIES; i++)
    {
        init_queue(&ready_TCB_queues[i]); 
    }

    init_queue(&dormant_TCB_queue);
}

//initialize HEAD and TAIL to NULL
void init_queue(TCB_queue *tcb_q) {
    tcb_q->HEAD = NULL;
    tcb_q->TAIL = NULL;
} 

int prio_to_array_index(U8 prio)
{
    if (prio == PRIO_NULL) {
        return 0;
    } else {
        // 84 + 1 - prio
        return LOWEST + 1 - prio;
    }
}

// @TODO mfmansoo | check if we can get the value from the array and do a shifty boi
void remove_tcb_given_tid(struct TCB_queue *q, task_t tid)
{
    TCB *tcb = &g_tcbs[tid];
    int isHead = q->HEAD->tid == tid; 
    int isTail = q->TAIL->tid == tid; 


    if (isHead) {
        q->HEAD = tcb->next;
        if (q->HEAD) {
            q->HEAD->prev = NULL;
        }
    }

    if (isTail) {
       // This TCB is the tail 
        q->TAIL = tcb->prev;
        if (q->TAIL) {
            q->TAIL->next = NULL;
        }
    } 
    
    if (!isHead && !isTail)  {
        // TCB is in the middle
        TCB *prev = tcb->prev;
        TCB *next = tcb->next;

        prev->next = next;
        next->prev = prev;
    }
}

TCB *queue_pop(struct TCB_queue *q){
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
