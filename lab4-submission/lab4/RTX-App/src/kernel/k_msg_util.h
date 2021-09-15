#include "k_task_util.h"
#include "k_mem.h"
#include "k_task.h"

#define RB_OK 0
#define RB_ERR -1

#define MBX_OK 0
#define MBX_ERR -1

typedef struct ring_buffer 
{
	// The buffer to store the data in
	U8 *buf;

	// The length of the buffer
	U16 length;

	// The amount of bytes that have been written
	U16 bytes_written;
	
	// head -> where the stored messsages start 
	U16 read_idx;

	// tail -> where the messages end 
	U16 write_idx;

} ring_buffer;

typedef struct mailbox 
{
	int initialized;

	// Senders that are blocked on the buffer
	TCB_queue sender_waiting_queue;

	// Reader that is blocked | should only ever be one item
	TCB_queue reader_waiting_queue;

	// Buffer for the messages
	ring_buffer rb;
} mailbox;

void init_ring_buffer_queue(ring_buffer *p_rb, U8 *buf, U16 length);
U16 get_ring_buffer_free_bytes(ring_buffer *p_rb);
int peek_ring_buffer(ring_buffer* p_rb);
int push_ring_buffer(ring_buffer* p_rb, const void *buf, size_t len);
int pop_ring_buffer(ring_buffer *p_rb, void *buf);
int validate_send_message(task_t receiver_tid, const void *buf, mailbox *p_mbx);
int validate_receive_message(void *buf, size_t len, mailbox *p_mbx);

int init_mailbox(mailbox *p_mbx, size_t len);

U16 get_msg_length(const void *msg_buf);
int cleanup_mailbox(mailbox *p_mbx);
