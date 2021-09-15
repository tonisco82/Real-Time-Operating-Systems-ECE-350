/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTX LAB  
 *
 *                     Copyright 2020-2021 Yiqing Huang
 *                          All rights reserved.
 *---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------*/

/**************************************************************************/ /**
 * @file        k_msg.c
 * @brief       kernel message passing routines          
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

#include "k_msg.h"

mailbox mailboxes[MAX_TASKS] = {0};
mailbox mailbox_uart = {0};

// Holds the buffers of the blocked senders
// We need this to figure out how many senders to unblock
const void *blocked_sender_bufs[MAX_TASKS] = {0};

// Blocks a sender and updates the blocked_sender_buffers
void block_sender(mailbox *p_mbx, const void *buf) {
	task_t tid = gp_current_task->tid;
	blocked_sender_bufs[tid] = buf;
	block_task(&(p_mbx->sender_waiting_queue), BLK_SEND);
}



// you should be able to unblock as may tasks as possible 
void unblock_senders(mailbox *p_mbx, int preempt) {
	TCB *p_tcb_head = p_mbx->sender_waiting_queue.HEAD;

	if (p_tcb_head == NULL) {
		return;
	}

	const void *blocked_buf = blocked_sender_bufs[p_tcb_head->tid]; 
	int msg_len = get_msg_length(blocked_buf);
	
	// While there is still free space that can be used us it
	// and there are tasks that can be unblocked	
	while (get_ring_buffer_free_bytes(&(p_mbx->rb)) >= msg_len && p_tcb_head  != NULL) {
		push_ring_buffer(&(p_mbx->rb), blocked_buf, get_msg_length(blocked_buf));
		// Unblock the task, dont call the scheduler
		unblock_task(&(p_mbx->sender_waiting_queue), 0);
		
		// Go to the next task
		p_tcb_head = p_tcb_head->next;
		blocked_buf = blocked_sender_bufs[p_tcb_head->tid]; 
		msg_len = get_msg_length(blocked_buf);
	}

	// Run the scheduler with the current task being preempted involuntarily
	if (preempt)
	{
		k_tsk_run_new_helper(0);
	}
}


int k_mbx_create(size_t size) {
	task_t tid = gp_current_task->tid;
	return _k_mbx_create(size, tid);
}

// CHECKED
int _k_mbx_create(size_t size, task_t tid)
{
#ifdef DEBUG_0
	printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
	// task_t current_tid = gp_current_task->tid;
	
	mailbox *p_mbx = NULL; 
	if (tid == TID_UART) {
		p_mbx = &mailbox_uart;
	} else {
		p_mbx = &(mailboxes[tid]);
	}

	if (p_mbx->initialized != 0) {
		errno = EEXIST;
		return RTX_ERR;
	} 
	
	if (size < MIN_MSG_SIZE) {
		errno = EINVAL;
		return RTX_ERR;
	}


	int ret = init_mailbox(p_mbx, size);

	if (ret != MBX_OK) {
		errno = ENOMEM;
		return RTX_ERR;
	}

	return tid;
}

// CHECKED
// Sending does not care about the blocked messages or not, just if the message can be delivered
// If not block the task and then try to run the task again
int k_send_msg(task_t receiver_tid, const void *buf)
{
#ifdef DEBUG_0
	printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
	mailbox *p_mbx;
	if (receiver_tid == TID_UART)
	{
		p_mbx = &mailbox_uart;
	} else {
		p_mbx = &(mailboxes[receiver_tid]);
	}

	if (validate_send_message(receiver_tid, buf, p_mbx) != RTX_OK){
		return RTX_ERR;
	}

	int blocked_task = p_mbx->sender_waiting_queue.HEAD != NULL;

	if (!blocked_task) {
		int ret = push_ring_buffer(&(p_mbx->rb), buf, get_msg_length(buf));

		if (ret == RB_OK) {
			// Only unblocks if there is is a waiting task
			if (receiver_tid == TID_UART)
			{
				unblock_task(&(p_mbx->reader_waiting_queue), 0);
			} else {
				unblock_task(&(p_mbx->reader_waiting_queue), 1);
			}
		} else {
			block_sender(p_mbx, buf);
		}
	} else {
		block_sender(p_mbx, buf);
	}

	return RTX_OK;
}

int k_send_msg_nb(task_t receiver_tid, const void *buf){
	return _k_send_msg_nb(receiver_tid, buf, gp_current_task->tid);
}

// CHECKED
int _k_send_msg_nb(task_t receiver_tid, const void *buf, task_t sender)
{
#ifdef DEBUG_0
	printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */

	mailbox *p_mbx = &(mailboxes[receiver_tid]);

	if (validate_send_message(receiver_tid, buf, p_mbx) != RTX_OK){
		return RTX_ERR;
	}
	
	int blocked_task = p_mbx->sender_waiting_queue.HEAD != NULL;

	if (blocked_task) {
		errno = ENOSPC;
		return RTX_ERR;
	}

	int ret = push_ring_buffer(&(p_mbx->rb), buf, get_msg_length(buf));

	if (ret == RB_OK) {
		if(sender == TID_UART) {
			unblock_task(&(p_mbx->reader_waiting_queue), 0);
		} else {
			unblock_task(&(p_mbx->reader_waiting_queue), 1);
		}
		return RTX_OK;
	} else {
		errno = ENOSPC;
		return RTX_ERR;
	}
}

// CHECKED
int k_recv_msg(void *buf, size_t len)
{
#ifdef DEBUG_0
	printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
	mailbox *p_mbx = &(mailboxes[gp_current_task->tid]);
	if (validate_receive_message(buf, len, p_mbx) != RTX_OK){
		return RTX_ERR;
	}

	if (pop_ring_buffer(&(p_mbx->rb), buf) == RB_OK)
	{
		unblock_senders(p_mbx, 1);
		return RTX_OK;
	} else {
		block_task(&(p_mbx->reader_waiting_queue), BLK_RECV);
		if (validate_receive_message(buf, len, p_mbx) != RTX_OK){
			return RTX_ERR;
		}
		pop_ring_buffer(&(p_mbx->rb), buf);
	}
	
	return RTX_OK;
}

int k_recv_msg_nb(void *buf, size_t len){
	return _k_recv_msg_nb(buf, len, gp_current_task->tid);
}

// CHECKED
int _k_recv_msg_nb(void *buf, size_t len, task_t receiver)
{
#ifdef DEBUG_0
	printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
	mailbox *p_mbx;
	if (receiver == TID_UART)
	{
		p_mbx = &mailbox_uart;
	} else {
		p_mbx = &(mailboxes[receiver]);
	}
	
	
	if (validate_receive_message(buf, len, p_mbx) != RTX_OK){
		return RTX_ERR;
	}

	if (pop_ring_buffer(&(p_mbx->rb), buf) == RB_OK)
	{
		if (receiver == TID_UART)
		{
			unblock_senders(p_mbx, 0);
		} else {
			unblock_senders(p_mbx, 1);
		}
		
		return RTX_OK;
	} else {
		errno = ENOMSG;
		return RTX_ERR;
	}
}

int k_mbx_ls(task_t *buf, size_t count)
{
#ifdef DEBUG_0
	printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
	if (buf == NULL)
	{
		errno = EFAULT;
		return RTX_ERR;
	}
	
	int num_mbxs = 0;
	for (size_t i = 0; i < MAX_TASKS; i++)
	{
		task_t tid = i;
		TCB *p_tcb = &(g_tcbs[tid]);
		if (check_task_valid(p_tcb) == RTX_OK && g_tcbs[i].state != DORMANT){
			if (mailboxes[tid].initialized){
				buf[num_mbxs] = tid;
				num_mbxs++;
			}
		}
		if (num_mbxs == count){
			return count;
		}
	}

	return num_mbxs;
}

// @TODO change this to work for UART
int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
	printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */
	mailbox mbx = mailboxes[tid];
	TCB *p_tcb = &(g_tcbs[tid]);

	if (check_task_valid(p_tcb) != RTX_OK || mbx.initialized != 1) {
		errno = ENOENT;
		return RTX_ERR;
	}

	return mbx.rb.length - mbx.rb.bytes_written;
}


void unblock_all_senders(mailbox *p_mbx) {
	while (p_mbx->sender_waiting_queue.HEAD != NULL) {
		unblock_task(&(p_mbx->sender_waiting_queue), 0);
	}
}

// NOOP if there is no mailbox 
int k_mbx_cleanup(void) {
	task_t current_tid = gp_current_task->tid;
	mailbox *p_mbx = &(mailboxes[current_tid]);

	if (p_mbx->initialized != 1) {
		return RTX_ERR;
	}

	unblock_all_senders(p_mbx);

	return cleanup_mailbox(p_mbx);
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
