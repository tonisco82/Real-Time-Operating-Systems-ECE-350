#include "k_msg_util.h"
#include "k_inc.h"

void init_ring_buffer_queue(ring_buffer *p_rb, U8 *buf, U16 length) {
	p_rb->read_idx = 0;
	p_rb->write_idx = 0;

	p_rb->buf = buf;
	p_rb->length = length;
}

U16 get_ring_buffer_free_bytes(ring_buffer *p_rb) {
	return p_rb->length - p_rb->bytes_written;
}


void read_buffer(ring_buffer *p_rb, void *dest, int start, int length) {
	int read_head_idx = start;

	for (int i = 0; i < length; i++) {
		((U8 *)dest)[i] = p_rb->buf[read_head_idx];
		read_head_idx = (read_head_idx + 1) % p_rb->length;	
	}

	return;
}



int peek_ring_buffer(ring_buffer* p_rb) {
	if (p_rb->bytes_written == 0) {
		return RB_ERR;
	}

	// @TODO change this to wrap around???
	// const void * p_msg = (const void *)&(p_rb->buf[p_rb->read_idx]);
	RTX_MSG_HDR msg_hdr = {0};

	read_buffer(p_rb, (void *)(&msg_hdr), p_rb->read_idx, MSG_HDR_SIZE);


	return get_msg_length(&msg_hdr);
}

U16 get_msg_length(const void *msg_buf) {
	RTX_MSG_HDR *hdr = (RTX_MSG_HDR *)msg_buf;
	return hdr->length;
}


int push_ring_buffer(ring_buffer* p_rb, const void *buf, size_t len) {
	// We cannot fit the message into the buffer
	if (len + p_rb->bytes_written > p_rb->length) {
		return RB_ERR;
	}

	// Start writing from the write_idx
	int write_head_idx = p_rb->write_idx;

	for (int i = 0; i < len; i++) {
		p_rb->buf[write_head_idx] = ((U8 *)buf)[i];

		// Loop over if the index is bigger than the len
		write_head_idx = (write_head_idx + 1) % p_rb->length;
	}

	// Increment the bytes written so we can check for full vs empty
	p_rb->bytes_written += len;

	// Allow the index to loop over
	p_rb->write_idx = (p_rb->write_idx + len) % p_rb->length;

	return RB_OK;

}

int pop_ring_buffer(ring_buffer *p_rb, void *buf) {
	// Check if the ring buffer is empty
	if (p_rb->bytes_written == 0) {
		return RB_ERR;
	}

	int length = peek_ring_buffer(p_rb);

	// int read_head_idx = p_rb->read_idx;
	read_buffer(p_rb, buf, p_rb->read_idx, length);

	// for (int i = 0; i < length; i++) {
	// 	((U8 *)buf)[i] = p_rb->buf[read_head_idx];
	// 	read_head_idx = (read_head_idx + 1) % p_rb->length;	
	// }

	// Decrement because those bytes can be written to again
	p_rb->bytes_written -= length;

	// Move read_idx forward to the next message 
	p_rb->read_idx = (p_rb->read_idx + length) % p_rb->length;
	return RB_OK;
}

int init_mailbox(mailbox *p_mbx, size_t len)  {
	init_queue(&(p_mbx->sender_waiting_queue));
	init_queue(&(p_mbx->reader_waiting_queue));

	p_mbx->initialized = 1;

	// Use the kernel memory
	void *buf = k_mpool_alloc(MPID_IRAM2, len);

	if (buf == NULL) {
		return MBX_ERR;
	}

	init_ring_buffer_queue(&(p_mbx->rb), (U8 *)buf, len);

	return MBX_OK;
}

int cleanup_mailbox(mailbox *p_mbx) {
	p_mbx->initialized = 0;
	return k_mpool_dealloc(MPID_IRAM2, p_mbx->rb.buf);
}


int validate_send_message(task_t receiver_tid, const void *buf, mailbox *p_mbx) {
	if (buf == NULL)
	{
		errno = EFAULT;		
		return RTX_ERR;
	}
	
	TCB *tcb = &g_tcbs[receiver_tid];
	// CHECKED OK 
	U16 msg_size = get_msg_length(buf);
	if (receiver_tid != TID_UART)
	{
		if (check_task_valid(tcb))
		{
			errno = EINVAL;
			return RTX_ERR;
		}
	}
	
	if (msg_size < MIN_MSG_SIZE)
	{
		errno = EINVAL;
		return RTX_ERR;
	}

	if (!p_mbx->initialized){
		errno = ENOENT;
		return RTX_ERR;
	}

	if (msg_size > p_mbx->rb.length) {
		errno = EMSGSIZE;
		return RTX_ERR;
	}

	return RTX_OK;
}

int validate_receive_message(void *buf, size_t len, mailbox *p_mbx) {
	if (buf == NULL)
	{
		errno = EFAULT;
		return RTX_ERR;
	}

	if (!p_mbx->initialized){
		errno = ENOENT;
		return RTX_ERR;
	}

	int res = peek_ring_buffer(&(p_mbx->rb));

	// RB_ERR indicates that the buffer is empty 
	// so we check if the buffer is not empty and the
	// length of the message is greater then the recv buf
	if (res != RB_ERR && res > len)
	{
		errno = ENOSPC;
		return RTX_ERR;
	}

	return RTX_OK;
}

