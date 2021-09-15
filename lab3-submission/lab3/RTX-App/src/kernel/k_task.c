/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2021 Yiqing Huang
 *                          All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/**************************************************************************/ /**
 * @file        k_task.c
 * @brief       task management C file
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only three simple tasks and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_task.h"
#include "k_rtx.h"
#include "k_task_util.h"

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB *gp_current_task = NULL; // the current RUNNING task
TCB g_tcbs[MAX_TASKS];		 // an array of TCBs
//TASK_INIT       g_null_task_info;           // The null task info
U32 g_num_active_tasks = 0; // number of non-dormant tasks

// task_t					num_total_tasks = 0;				//total number of tasks (for g_tcbs[])

U8 g_largest_tid = 0; // Largest tid that has been used

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
             g_k_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

/**************************************************************************/ /**
 * @brief   	SVC Handler
 * @pre         PSP is used in thread mode before entering SVC Handler
 *              SVC_Handler is configured as the highest interrupt priority
 *****************************************************************************/

void SVC_Handler(void)
{

	U8 svc_number;
	U32 ret = RTX_OK;				// default return value of a function
	U32 *args = (U32 *)__get_PSP(); // read PSP to get stacked args

	svc_number = ((S8 *)args[6])[-2]; // Memory[(Stacked PC) - 2]
	switch (svc_number)
	{
	case SVC_RTX_INIT:
		ret = k_rtx_init((RTX_SYS_INFO *)args[0], (TASK_INIT *)args[1], (int)args[2]);
		break;
	case SVC_MEM_ALLOC:
		ret = (U32)k_mpool_alloc(MPID_IRAM1, (size_t)args[0]);
		break;
	case SVC_MEM_DEALLOC:
		ret = k_mpool_dealloc(MPID_IRAM1, (void *)args[0]);
		break;
	case SVC_MEM_DUMP:
		ret = k_mpool_dump(MPID_IRAM1);
		break;
	case SVC_TSK_CREATE:
		ret = k_tsk_create((task_t *)(args[0]), (void (*)(void))(args[1]), (U8)(args[2]), (U32)(args[3]));
		break;
	case SVC_TSK_EXIT:
		k_tsk_exit();
		break;
	case SVC_TSK_YIELD:
		ret = k_tsk_yield();
		break;
	case SVC_TSK_SET_PRIO:
		ret = k_tsk_set_prio((task_t)args[0], (U8)args[1]);
		break;
	case SVC_TSK_GET:
		ret = k_tsk_get((task_t)args[0], (RTX_TASK_INFO *)args[1]);
		break;
	case SVC_TSK_GETTID:
		ret = k_tsk_gettid();
		break;
	case SVC_TSK_LS:
		ret = k_tsk_ls((task_t *)args[0], (size_t)args[1]);
		break;
	case SVC_MBX_CREATE:
		ret = k_mbx_create((size_t)args[0]);
		break;
	case SVC_MBX_SEND:
		ret = k_send_msg((task_t)args[0], (const void *)args[1]);
		break;
	case SVC_MBX_SEND_NB:
		ret = k_send_msg_nb((task_t)args[0], (const void *)args[1]);
		break;
	case SVC_MBX_RECV:
		ret = k_recv_msg((void *)args[0], (size_t)args[1]);
		break;
	case SVC_MBX_RECV_NB:
		ret = k_recv_msg_nb((void *)args[0], (size_t)args[1]);
		break;
	case SVC_MBX_LS:
		ret = k_mbx_ls((task_t *)args[0], (size_t)args[1]);
		break;
	case SVC_MBX_GET:
		ret = k_mbx_get((task_t)args[0]);
		break;
	default:
		ret = (U32)RTX_ERR;
	}

	args[0] = ret; // return value saved onto the stacked R0
}

/**************************************************************************/ /**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/

TCB *scheduler(void)
{
	if (ready_TCB_queues[4].HEAD != NULL)
	{
		return &g_tcbs[ready_TCB_queues[4].HEAD->tid];
	}
	else if (ready_TCB_queues[3].HEAD != NULL)
	{
		return &g_tcbs[ready_TCB_queues[3].HEAD->tid];
	}
	else if (ready_TCB_queues[2].HEAD != NULL)
	{
		return &g_tcbs[ready_TCB_queues[2].HEAD->tid];
	}
	else if (ready_TCB_queues[1].HEAD != NULL)
	{
		return &g_tcbs[ready_TCB_queues[1].HEAD->tid];
	}
	else
	{
		return &g_tcbs[ready_TCB_queues[0].HEAD->tid];
	}
}

/**
 * @brief initialize the first task in the system
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
	p_task->prio = PRIO_NULL;
	p_task->priv = 0;
	p_task->tid = TID_NULL;
	p_task->ptask = &task_null;
	p_task->u_stack_size = PROC_STACK_SIZE;
}

/**
 * @brief initialize the KCD and CON tasks in the system
 */
void k_tsk_init_daemons()
{
	TASK_INIT kcd;
	TASK_INIT con;
	TASK_INIT *p_task_kcd = &kcd;
	TASK_INIT *p_task_con = &con;

	p_task_kcd->prio = HIGH;
	p_task_kcd->priv = 0;
	p_task_kcd->tid = TID_KCD;
	p_task_kcd->ptask = &task_kcd;
	p_task_kcd->u_stack_size = 0x300;
	p_task_con->prio = HIGH;
	p_task_con->priv = 1;
	p_task_con->tid = TID_CON;
	p_task_con->ptask = &task_cdisp;
	p_task_con->u_stack_size = PROC_STACK_SIZE;

	if (k_tsk_create_new(p_task_kcd, &g_tcbs[TID_KCD], TID_KCD) == RTX_OK)
	{
		g_num_active_tasks++;
		enqueue_back(&ready_TCB_queues[prio_to_array_index(g_tcbs[TID_KCD].prio)], &g_tcbs[TID_KCD]);
	}

	if (k_tsk_create_new(p_task_con, &g_tcbs[TID_CON], TID_CON) == RTX_OK)
	{
		g_num_active_tasks++;
		enqueue_back(&ready_TCB_queues[prio_to_array_index(g_tcbs[TID_CON].prio)], &g_tcbs[TID_CON]);
	}
}

/**************************************************************************/ /**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init(TASK_INIT *task, int num_tasks)
{
	// Initialize the queues
	init_queues();

	// Because we have the null task we can at most add 15 tasks from num_tasks
	if (num_tasks > MAX_TASKS - 3)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	TASK_INIT taskinfo;

	k_tsk_init_first(&taskinfo);
	k_tsk_init_daemons();
	_k_mbx_create(UART_MBX_SIZE, TID_UART);
	if (k_tsk_create_new(&taskinfo, &g_tcbs[TID_NULL], TID_NULL) == RTX_OK)
	{
		g_num_active_tasks = 1;
		enqueue_back(&ready_TCB_queues[prio_to_array_index(g_tcbs[TID_NULL].prio)], &g_tcbs[TID_NULL]);
	}
	else
	{
		g_num_active_tasks = 0;
		errno = EINVAL;
		return RTX_ERR;
	}

	// create the rest of the tasks
	for (int i = 0; i < num_tasks; i++)
	{
		// It is okay to use the TIDs like this because this runs before anything else
		TCB *p_tcb = &g_tcbs[i + 1];

		if (k_tsk_create_new(&task[i], p_tcb, i + 1) == RTX_OK)
		{
			g_num_active_tasks++;
			enqueue_back(&ready_TCB_queues[prio_to_array_index(p_tcb->prio)], p_tcb);
		}
		else
		{
			return RTX_ERR;
		}
	}

	// Set the highest priority task to be the one that runs
	gp_current_task = scheduler();

	// Remove it from the queue as its now the "running" task
	remove_tcb_given_tid(&ready_TCB_queues[prio_to_array_index(gp_current_task->prio)], gp_current_task->tid);

	// We can do this because the NULL task has a static tid of 0
	// and the above for loop allocates based on i + 1
	g_largest_tid = num_tasks;

	return RTX_OK;
}
/**************************************************************************/ /**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
	extern U32 SVC_RTE;

	U32 *usp;
	U32 *ksp;

	if (p_taskinfo == NULL || p_tcb == NULL)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	if ((p_taskinfo->prio < HIGH || p_taskinfo->prio > LOWEST) && tid != TID_NULL)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	p_tcb->tid = tid;
	p_tcb->state = READY;
	p_tcb->prio = p_taskinfo->prio;
	p_tcb->priv = p_taskinfo->priv;
	p_tcb->ptask = p_taskinfo->ptask;
	p_tcb->u_stack_size = p_taskinfo->u_stack_size < PROC_STACK_SIZE ? PROC_STACK_SIZE : p_taskinfo->u_stack_size;

	/*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * ATTENTION: you need to modify the following three lines of code
     *            so that you use your own dynamic memory allocator
     *            to allocate variable size user stack.
     * -------------------------------------------------------------*/

	usp = (U32 *)k_mpool_alloc(MPID_IRAM2, p_tcb->u_stack_size);

	// Store so we can deallocate ez
	p_tcb->u_sp_end = usp;

	//moves usp to correct position (going down)
	usp = (U32 *)((U8 *)usp + p_tcb->u_stack_size);

	if ((U32)usp & 0x04)
	{		   // if sp not 8B aligned, then it must be 4B aligned
		usp--; // adjust it to 8B aligned
	}

	// Store the base address of the USP
	p_tcb->u_sp_base = usp;

	if (usp == NULL)
	{
		errno = ENOMEM;
		return RTX_ERR;
	}

	/*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

	// if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
	// since we never enter from SVC handler in this case

	*(--usp) = INITIAL_xPSR;			 // xPSR: Initial Processor State
	*(--usp) = (U32)(p_taskinfo->ptask); // PC: task entry point

	// uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
	for (int j = 0; j < 6; j++)
	{
#ifdef DEBUG_0
		*(--usp) = 0xDEADAAA0 + j;
#else
		*(--usp) = 0x0;
#endif
	}

	// allocate kernel stack for the task
	ksp = k_alloc_k_stack(tid);
	if (ksp == NULL)
	{
		errno = ENOMEM;
		return RTX_ERR;
	}

	/*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
	// a task never run before directly exit
	*(--ksp) = (U32)(&SVC_RTE);
	// kernel stack R4 - R12, 9 registers
#define NUM_REGS 9 // number of registers to push
	for (int j = 0; j < NUM_REGS; j++)
	{
#ifdef DEBUG_0
		*(--ksp) = 0xDEADCCC0 + j;
#else
		*(--ksp) = 0x0;
#endif
	}

	// put user sp on to the kernel stack
	*(--ksp) = (U32)usp;

	// save control register so that we return with correct access level
	if (p_taskinfo->priv == 1)
	{ // privileged
		*(--ksp) = __get_CONTROL() & ~BIT(0);
	}
	else
	{ // unprivileged
		*(--ksp) = __get_CONTROL() | BIT(0);
	}

	p_tcb->msp = ksp;

	// Store the USP after the changes have been made
	p_tcb->usp = usp;

	return RTX_OK;
}

/**************************************************************************/ /**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
        STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}


__asm void k_tsk_start(void)
{
		PRESERVE8
		B K_RESTORE
}

/**************************************************************************/ /**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(int old_task_state)
{
	TCB *p_tcb_old = NULL;

	if (gp_current_task == NULL)
	{
		return RTX_ERR;
	}

	p_tcb_old = gp_current_task;
	gp_current_task = scheduler();

	if (gp_current_task == NULL)
	{
		gp_current_task = p_tcb_old; // revert back to the old task

		// The old task is still "running"
		remove_tcb_given_tid(&ready_TCB_queues[prio_to_array_index(gp_current_task->prio)], gp_current_task->tid);
		return RTX_ERR;
	}

	// Remove the task that is going to run from the ready queue
	remove_tcb_given_tid(&ready_TCB_queues[prio_to_array_index(gp_current_task->prio)], gp_current_task->tid);
	// at this point, gp_current_task != NULL and p_tcb_old != NULL
	if (gp_current_task != p_tcb_old)
	{
		gp_current_task->state = RUNNING; // change state of the to-be-switched-in  tcb

		p_tcb_old->state = old_task_state; // change state of the to-be-switched-out tcb

		k_tsk_switch(p_tcb_old); // switch kernel stacks
	}

	return RTX_OK;
}

/**************************************************************************/ /**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{
	// errno should not be set
	errno = 0;
	return k_tsk_run_new_helper(1);
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid(void)
{
	return gp_current_task->tid;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMENTED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
	printf("k_tsk_create: entering...\n\r");
	printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */
	// Get the tid that you are going to use for the new task
	task_t tid = get_next_tid();

	// The tid is not valid and we should not trust it
	if (tid == INVALID_TID)
	{
		errno = EAGAIN;
		return RTX_ERR;
	}

	if (prio < HIGH || prio > LOWEST)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	TCB *tcb = &g_tcbs[tid];

	// Create the task initialization structure
	// Note: we enforce a min user stack size

	TASK_INIT new_task_init;
	new_task_init.tid = tid;
	new_task_init.ptask = task_entry;
	new_task_init.u_stack_size = (stack_size < PROC_STACK_SIZE) ? PROC_STACK_SIZE : stack_size;
	new_task_init.prio = prio;
	new_task_init.priv = 0;

	// // Initialize the TCB and create the dummy frames
	k_tsk_create_new(&new_task_init, tcb, tid);

	// Put this onto the back of the queue
	enqueue_back(&ready_TCB_queues[prio_to_array_index(prio)], tcb);

	// Set the pointer to be TID according to the spec
	*task = tid;

	g_num_active_tasks++;

	// Run the scheduler
	k_tsk_run_new_helper(0);

	return RTX_OK;
}

int k_tsk_run_new_helper(int voluntary)
{
	TCB *p_tcb = gp_current_task;
	if (voluntary)
	{
		enqueue_back(&ready_TCB_queues[prio_to_array_index(p_tcb->prio)], p_tcb);
	}
	else
	{
		enqueue_front(&ready_TCB_queues[prio_to_array_index(p_tcb->prio)], p_tcb);
	}

	return k_tsk_run_new(READY);
}

void k_tsk_exit(void)
{
#ifdef DEBUG_0
	printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
	// Note: the TCB for the exiting task must NOT be in a ready queue
	TCB *p_tcb = gp_current_task;

	k_mpool_dealloc(MPID_IRAM2, p_tcb->u_sp_end);

	// Put this on the dormant queue, so we can reuse TCBs quickly
	enqueue_back(&dormant_TCB_queue, p_tcb);

	// There is one less running task, so we can decrement this
	g_num_active_tasks--;

	// Clean up the mailbox if there is one 
	k_mbx_cleanup();

	// set the state to Dormant as k_tsk_run_new() sets it to ready
	// p_tcb->state = DORMANT;
	k_tsk_run_new(DORMANT);

	return;
}

int k_tsk_set_prio(task_t task_id, U8 prio)
{
#ifdef DEBUG_0
	printf("k_tsk_set_prio: entering...\n\r");
	printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */
	// Cannot change a task's prio to PRIO_NULL
	if (prio == PRIO_NULL)
	{
		errno = EINVAL;
		return RTX_ERR;
	}
	if (task_id == TID_NULL)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	TCB *p_running_TCB = gp_current_task;
	TCB *p_update_TCB = &g_tcbs[task_id];

	if (p_running_TCB->priv == 0 && p_update_TCB->priv == 1)
	{
		errno = EPERM; // Per piazza 169
		return RTX_ERR;
	}

	if (check_task_valid(p_update_TCB) != RTX_OK || p_update_TCB->state == DORMANT ||
		prio < HIGH || prio > LOWEST)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	U8 prev_prio = prio_to_array_index(p_update_TCB->prio);
	// New prio is same as old prio No action is performed
	if (prio_to_array_index(prio) == prev_prio)
	{
		return RTX_OK;
	}

	// Update the priority of the task
	p_update_TCB->prio = prio;

	if (p_running_TCB->tid != p_update_TCB->tid && p_update_TCB->state == READY)
	{
		// Remove from previous queue it was in.
		remove_tcb_given_tid(&ready_TCB_queues[prev_prio], task_id);

		// Move to the respective queue of task's new prio.
		enqueue_back(&ready_TCB_queues[prio_to_array_index(prio)], p_update_TCB);
	}

	k_tsk_run_new_helper(0);

	return RTX_OK;
}

/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
	printf("k_tsk_get: entering...\n\r");
	printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */
	if (buffer == NULL)
	{
		errno = EFAULT;
		return RTX_ERR;
	}

	TCB tcb = g_tcbs[tid];

	if (check_task_valid(&tcb) != RTX_OK)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	U32 msp = NULL;
	U32 psp = NULL;

	if (tid == gp_current_task->tid)
	{
		psp = __get_PSP();
		msp = __get_MSP();
	}
	else
	{
		psp = (U32)tcb.usp;
		msp = (U32)tcb.msp;
	}

	buffer->tid = tid;
	buffer->prio = tcb.prio;
	buffer->u_stack_size = tcb.u_stack_size;
	buffer->priv = tcb.priv;
	buffer->ptask = tcb.ptask;
	buffer->k_sp = msp;
	buffer->k_sp_base = (U32)k_alloc_k_stack(tid);
	buffer->k_stack_size = g_k_stack_size;
	buffer->state = tcb.state;
	buffer->u_sp = psp;
	buffer->u_sp_base = (U32)tcb.u_sp_base;

	return RTX_OK;
}

int k_tsk_ls(task_t *buf, size_t count)
{
#ifdef DEBUG_0
	printf("k_tsk_ls: buf=0x%x, count=%d\r\n", buf, count);
#endif /* DEBUG_0 */
	if (buf == NULL)
	{
		errno = EFAULT;
		return RTX_ERR;
	}
	
	int num_tasks = 0;
	for (size_t i = 0; i < MAX_TASKS; i++)
	{
		task_t tid = i;
		TCB *p_tcb = &(g_tcbs[tid]);
		if (check_task_valid(p_tcb) == RTX_OK && g_tcbs[i].state != DORMANT){
			buf[num_tasks] = tid;
			num_tasks++;
		}
		if (num_tasks == count){
			return count;
		}
	}
	return num_tasks;
}

task_t get_next_tid(void)
{
	if (dormant_TCB_queue.HEAD != NULL)
	{	
		// Take the tid from the dormant queue;
		return queue_pop(&dormant_TCB_queue)->tid;
	}
	else
	{
		// @TODO mfmansoo | change this to be -3 to take into account the console task and the kcd task
		if (g_largest_tid == MAX_TASKS - 3)
		{
			return INVALID_TID;
		}
		else
		{
			// CHECK THIS BAD BOI
			return ++g_largest_tid;
		}
	}
}


int check_task_valid(TCB *p_tcb) {
	if (p_tcb == NULL || (p_tcb->tid > g_largest_tid && p_tcb->tid != TID_KCD && p_tcb->tid != TID_CON)) {
		return RTX_ERR;
	}

	return RTX_OK;
}


// Take the current task and block it
int block_task(TCB_queue *waiting_queue, int blocked_state) {
	TCB *p_tcb = gp_current_task;

	// Ensure that the task is valid 
	if (check_task_valid(p_tcb) != RTX_OK) {
		return RTX_ERR;
	}

	// Set the state to be blocked
	p_tcb->state = blocked_state;

	// Push the blocked task to the back of the queue
	enqueue_back(waiting_queue, p_tcb);

	// Get the next task to run
	if (k_tsk_run_new(blocked_state) != RTX_OK) {
		return RTX_ERR;
	}

	return RTX_OK;
}


// NOOP if there is no task to unblock
int unblock_task(TCB_queue *waiting_queue, U8 run_scheduler) {
	if (waiting_queue->HEAD == NULL) {
		return RTX_ERR;
	}

	// Get the next tcb to unblock 
	TCB *p_tcb = queue_pop(waiting_queue);

	// Change the state to ready
	p_tcb->state = READY;

	// Push it to the back of the ready queue 
	enqueue_back(&ready_TCB_queues[prio_to_array_index(p_tcb->prio)], p_tcb);

	if (run_scheduler)  {
		if (k_tsk_run_new_helper(0) != RTX_OK) {
			return RTX_ERR;
		}
	}

	return RTX_OK;
} 

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
