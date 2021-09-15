
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
 * @file        ae_tasks300.c
 * @brief       lab3 Test Suite 300  - Basic Non-blocking Message Passing
 *
 * @version     V1.2021.07
 * @authors     Yiqing Huang
 * @date        2021 Jul
 *
 * @note        Each task is in an infinite loop. These Tasks never terminate.
 *
 *****************************************************************************/

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define NUM_RUNS 2
#define NUM_TESTS NUM_RUNS * 2		 // number of tests
#define NUM_INIT_TASKS 1 // number of tasks during initialization
#define BUF_LEN 80		 // receiver buffer length
#define MY_MSG_TYPE 100	 // some customized message type

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
const char PREFIX[] = "G10-TS3";
const char PREFIX_LOG[] = "G10-TS3-LOG";
const char PREFIX_LOG2[] = "G10-TS3-LOG2";
TASK_INIT g_init_tasks[NUM_INIT_TASKS];

AE_XTEST g_ae_xtest; // test data, re-use for each test
AE_CASE g_ae_cases[NUM_TESTS];
AE_CASE_TSK g_tsk_cases[NUM_TESTS];

/* The following arrays can also be dynamic allocated to reduce ZI-data size
 *  They do not have to be global buffers (provided the memory allocator has no bugs)
 */

U8 g_buf1[BUF_LEN];
U8 g_buf2[BUF_LEN];
U8 g_buf3[BUF_LEN];

task_t g_tasks[MAX_TASKS];
task_t g_tids[MAX_TASKS];
int g_test_id;

void set_ae_init_tasks(TASK_INIT **pp_tasks, int *p_num)
{
	*p_num = NUM_INIT_TASKS;
	*pp_tasks = g_init_tasks;
	set_ae_tasks(*pp_tasks, *p_num);
}

void set_ae_tasks(TASK_INIT *tasks, int num)
{
	for (int i = 0; i < num; i++)
	{
		tasks[i].u_stack_size = PROC_STACK_SIZE;
		tasks[i].prio = HIGH;
		tasks[i].priv = 0;
	}

	tasks[0].ptask = &task0;

	init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
	g_ae_xtest.test_id = 0;
	g_ae_xtest.index = 0;
	g_ae_xtest.num_tests = NUM_TESTS;
	g_ae_xtest.num_tests_run = 0;

	for (int i = 0; i < NUM_TESTS; i++)
	{
		g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
		g_tsk_cases[i].p_ae_case->results = 0x0;
		g_tsk_cases[i].p_ae_case->test_id = i;
		g_tsk_cases[i].p_ae_case->num_bits = 0;
		g_tsk_cases[i].pos = 0; // first avaiable slot to write exec seq tid
		// *_expt fields are case specific, deligate to specific test case to initialize
	}
	printf("%s: START\r\n", PREFIX);
}

void update_ae_xtest(int test_id)
{
	g_ae_xtest.test_id = test_id;
	g_ae_xtest.index = 0;
	g_ae_xtest.num_tests_run++;
}

int update_exec_seq(int test_id, task_t tid)
{

	U8 len = g_tsk_cases[test_id].len;
	U8 *p_pos = &g_tsk_cases[test_id].pos;
	task_t *p_seq = g_tsk_cases[test_id].seq;
	p_seq[*p_pos] = tid;
	(*p_pos)++;
	(*p_pos) = (*p_pos) % len; // preventing out of array bound
	return RTX_OK;
}

void gen_req0(int test_id)
{
	g_tsk_cases[test_id].p_ae_case->num_bits = 32; // @TODO | Fix this
	g_tsk_cases[test_id].p_ae_case->results = 0;
	g_tsk_cases[test_id].p_ae_case->test_id = test_id;
	g_tsk_cases[test_id].len = MAX_LEN_SEQ;		 // assign a value no greater than MAX_LEN_SEQ
	g_tsk_cases[test_id].pos_expt = 17; // @TODO | Fix this

	update_ae_xtest(test_id);
}


void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:12] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = 18;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 0;       // N/A for this test
    g_tsk_cases[test_id].pos_expt = 0;  // N/A for this test
    
    update_ae_xtest(test_id);
}


// Message - string to be copied in
// Length - length of the string in bytes
void init_message(U8 *buf, int tid, const char *message, int length)
{
	size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
	struct rtx_msg_hdr *ptr = (void *)buf;
	ptr->length = msg_hdr_size + length; // set the message length
	ptr->type = DEFAULT;			// set message type
	ptr->sender_tid = tid;			// set sender id

	U8 *temp_buf = buf + msg_hdr_size;
	strcpy((char *)temp_buf, message);
	return;
}

// Checks to see if the message matches the expected data
// conforms to the RTX_err | RTX_ok syntax
int validate_message(U8 *buf, int expected_tid, task_t expected_type, const char *expected_message)
{
	RTX_MSG_HDR *ptr = (void *)buf;
	U32 length = ptr->length;

	if (ptr->sender_tid != expected_tid)
	{
		return RTX_ERR;
	}

	if (ptr->type != expected_type)
	{
		return RTX_ERR;
	}

	// Get the length of the data
	int data_length = length - MSG_HDR_SIZE;

	U8 *data_ptr = buf + MSG_HDR_SIZE;

	for (int i = 0; i < data_length; i++)
	{
		if (data_ptr[i] != expected_message[i])
		{
			return RTX_ERR;
		}
	}

	return RTX_OK;
}

// Automatically increments the p_index
void update_tests(int test_id, U8 *p_index, int sub_result, const char *message)
{
	strcpy(g_ae_xtest.msg, message);
	process_sub_result(test_id, *p_index, sub_result);
	(*p_index)++;
}

// Assumes that RTX_OK is a pass
void update_tests_rtx(int test_id, U8 *p_index, int ret_val, const char *message)
{
	int sub_result = ret_val == RTX_OK ? 1 : 0;
	update_tests(test_id, p_index, sub_result, message);
}

void exit_on_fail(int ret_val)
{
	if (ret_val != RTX_OK)
	{
		test_exit();
	}
}

int test0_start(int test_id)
{
	int ret_val = 0;

	// Initialize the datastructure that holds the results
	gen_req0(test_id);

	// Index of the current test
	U8 *p_index = &(g_ae_xtest.index);
	*p_index = 0;

	ret_val = tsk_create(&g_tids[1], &task1, HIGH, 0x200);
	update_tests_rtx(test_id, p_index, ret_val, "task0: creating a HIGH prio task that runs task1 function");
	exit_on_fail(ret_val);

	// Preempt and go to task 1
	task_t tid = tsk_gettid();
	ret_val = tsk_set_prio(tid, LOWEST);
	update_tests_rtx(test_id, p_index, ret_val, "task 0: set the priority to lowest");
	exit_on_fail(ret_val);

	// @TODO add the expected sequence
	return 0;
}



void set_expected_output(int test_id) {
    task_t  *p_seq_expt = g_tsk_cases[test_id].seq_expt;

	p_seq_expt[0]  = g_tids[0];
	p_seq_expt[1]  = g_tids[1];
	p_seq_expt[2]  = g_tids[2];
	p_seq_expt[3]  = g_tids[1];
	p_seq_expt[4]  = g_tids[2];
	p_seq_expt[5]  = g_tids[3];
	p_seq_expt[6]  = g_tids[2];
	p_seq_expt[7]  = g_tids[1];
	p_seq_expt[8]  = g_tids[2];
	p_seq_expt[9]  = g_tids[1];
	p_seq_expt[10] = g_tids[3];
	p_seq_expt[11] = g_tids[2];
	p_seq_expt[12] = g_tids[3];
	p_seq_expt[13] = g_tids[1];
	p_seq_expt[14] = g_tids[2];
	p_seq_expt[15] = g_tids[1];
	p_seq_expt[16] = g_tids[2];


}

int test1_start(int test_id, int test_id_data) {
	set_expected_output(test_id_data);
    gen_req1(test_id);

	U8      pos         = g_tsk_cases[test_id_data].pos;
    U8      pos_expt    = g_tsk_cases[test_id_data].pos_expt;
    task_t  *p_seq      = g_tsk_cases[test_id_data].seq;
    task_t  *p_seq_expt = g_tsk_cases[test_id_data].seq_expt;
       
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	*p_index = 0;

	    // output the real execution order
    printf("%s: real exec order: ", PREFIX_LOG);
    int pos_len = (pos > MAX_LEN_SEQ)? MAX_LEN_SEQ : pos;
    for ( int i = 0; i < pos_len; i++) {
        printf("%d -> ", p_seq[i]);
    }
    printf("NIL\r\n");

    // output the expected execution order
    printf("%s: expt exec order: ", PREFIX_LOG);
    for ( int i = 0; i < pos_expt; i++ ) {
        printf("%d -> ", p_seq_expt[i]);
    }
    printf("NIL\r\n");


	for (int i = 0; i < pos_expt; i++) {
		sub_result = p_seq_expt[i] == p_seq[i] ? 1 : 0;
		update_tests(test_id, p_index, sub_result, "Checking execution order");
	}


	sub_result = pos_expt == pos ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "Checking number of executions");

	return 0;
}

/*
Master task that creates all child tasks
*/
void task0(void)
{
	task_t tid = tsk_gettid();
	g_tids[0] = tid;

	for (int i = 0; i < NUM_RUNS; i++) {
		// Update the execution sequence
		g_test_id = i * 2;
		update_exec_seq(g_test_id, tid);
		test0_start(g_test_id);
		test1_start(g_test_id + 1, g_test_id);	
	}

	test_exit();
}

void task1(void)
{
	int test_id = g_test_id;
	task_t tid = tsk_gettid();

	int ret_val = 0;
	int sub_result = 0;
	U8 *p_index = &(g_ae_xtest.index);

	// Update the exec sequence
	update_exec_seq(test_id, tid);

	// Create a medium priority task 2
	ret_val = tsk_create(&g_tids[2], &task2, MEDIUM, 0x200);
	update_tests_rtx(test_id, p_index, ret_val, "task1: creating a MEDIUM prio task that runs task2 function");
	exit_on_fail(ret_val);

	// Create a mailbox of 80 bytes
	ret_val = mbx_create(BUF_LEN);
	sub_result = ret_val >= 0 ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "task1: creating a mailbox");

	// After this task 1 should be blocked
	ret_val = recv_msg(g_buf1, BUF_LEN);
	update_tests_rtx(test_id, p_index, ret_val, "task1:  receive from the mailbox");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task2
	update_exec_seq(test_id, tid);

	// Validate that the message is what we expect
	ret_val = validate_message(g_buf1, g_tids[2], DEFAULT, "MFM");
	update_tests_rtx(test_id, p_index, ret_val, "task1:  message from mailbox is the expected value");
	exit_on_fail(ret_val);

	// After this task 1 should be blocked again
	ret_val = recv_msg(g_buf1, BUF_LEN);

	// Update the exec sequence | should run after task2 -> task 3 -> task 2
	update_exec_seq(test_id, tid);


	// Validate that the message is what we expect
	ret_val = validate_message(g_buf1, g_tids[2], DEFAULT, "<3");
	update_tests_rtx(test_id, p_index, ret_val, "task1:  message from mailbox is the expected value");
	exit_on_fail(ret_val);


	U8 *buf = &g_buf1[0];
	// Total len = 70 bytes
	// After this we should stay in task 1 
	init_message(buf, tid, "888888888888888888888888888888888888888888888888888888888888888", 64);
	ret_val = send_msg(g_tids[3], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task1: sending a message to task 3");
	exit_on_fail(ret_val);

	// After this we should go to task 2
	ret_val = tsk_set_prio(g_tids[1], LOW);
	update_tests_rtx(test_id, p_index, ret_val, "task1: setting own priority to low");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task2 
	update_exec_seq(test_id, tid);

	// Total len = 70 bytes
	// After this we should be blocked
	init_message(buf, tid, "3L-1", 5);
	ret_val = send_msg(g_tids[3], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task1: sending a message to task 3");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);

	// Total size: 20 bytes
	init_message(buf, tid, "=3=3=3=3=3=3=", 14);
	ret_val = send_msg(g_tids[2], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task1: sending a message to task 2");
	exit_on_fail(ret_val);

	// After this the task should be blocked
	// Total bytes 65
	init_message(buf, tid, "9999999999999999999999999999999999999999999999999999999999", 59);
	ret_val = send_msg(g_tids[2], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task1: sending a message to task 2");
	exit_on_fail(ret_val);

	
		// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);
	
	// Total size: 20 bytes
	init_message(buf, tid, "sl", 3);
	ret_val = send_msg_nb(g_tids[2], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task1: sending a message to task 2");
	exit_on_fail(ret_val);

	// Total size: 20 bytes
	init_message(buf, tid, "ls", 3);
	ret_val = send_msg_nb(g_tids[2], (void *)buf);
	sub_result = ret_val != RTX_OK ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "task1: sending a message to task 2 failing");

	tsk_exit();
}






void task2(void)
{
	int test_id = g_test_id;
	task_t tid = tsk_gettid();

	int ret_val = 0;
	int sub_result = 0;
	U8 *p_index = &(g_ae_xtest.index);

	// Update the exec sequence
	update_exec_seq(test_id, tid);

	// Create a medium priority task 2
	ret_val = tsk_create(&g_tids[3], &task3, LOW, 0x200);
	update_tests_rtx(test_id, p_index, ret_val, "task2: creating a MEDIUM prio task that runs task3 function");
	exit_on_fail(ret_val);

	// After this we should go back to task 1
	U8 *buf = &g_buf2[0];
	init_message(buf, tid, "MFM", 4);
	ret_val = send_msg_nb(g_tids[1], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task2: sending a message to task 1");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);

	// Create a mailbox of 80 bytes
	ret_val = mbx_create(BUF_LEN);
	sub_result = ret_val >= 0 ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "task2: creating a mailbox");

	// After this task 2 should be blocked
	ret_val = recv_msg(g_buf2, BUF_LEN);

	// Update the exec sequence | should run after task 3 sends a message
	update_exec_seq(test_id, tid);



	// Validate that the message is what we expect
	ret_val = validate_message(g_buf2, g_tids[3], DEFAULT, "MFMALEX");
	update_tests_rtx(test_id, p_index, ret_val, "task2:  message from mailbox is the expected value");
	exit_on_fail(ret_val);


	// Send a message to task 1
	// After this we should go back to task 1
	init_message(buf, tid, "<3", 3);
	ret_val = send_msg_nb(g_tids[1], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task2: sending a message to task 1");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task 3 sends a message
	update_exec_seq(test_id, tid);

	// Send a message > 10 bytes to task 3 to block this task
	// Total size = 17 bytes 
	// After this we should stay in task 1 
	init_message(buf, tid, "3333333333", 11);
	ret_val = send_msg(g_tids[3], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task2: sending a message to task 3");
	exit_on_fail(ret_val);



	// Update the exec sequence | should run after task 3 receives a message 
	update_exec_seq(test_id, tid);


	// After this we should go to task 3
	ret_val = tsk_set_prio(g_tids[2], LOWEST);
	update_tests_rtx(test_id, p_index, ret_val, "task2: setting own priority to lowest");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);

	// After this task 1 should be unblocked
	ret_val = recv_msg_nb(g_buf2, BUF_LEN);
	update_tests_rtx(test_id, p_index, ret_val, "task2:  receive from the mailbox");
	exit_on_fail(ret_val);

	// After this task 1 should run
	ret_val = validate_message(g_buf2, g_tids[1], DEFAULT, "=3=3=3=3=3=3=");
	update_tests_rtx(test_id, p_index, ret_val, "task2:  message from mailbox is the expected value");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);

	ret_val = recv_msg_nb(g_buf2, BUF_LEN);
	update_tests_rtx(test_id, p_index, ret_val, "task2:  receive from the mailbox");
	exit_on_fail(ret_val);

	ret_val = recv_msg_nb(g_buf2, BUF_LEN);
	update_tests_rtx(test_id, p_index, ret_val, "task2:  receive from the mailbox");
	exit_on_fail(ret_val);

	ret_val = recv_msg_nb(g_buf2, BUF_LEN);
	sub_result = ret_val != RTX_OK ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "task2:  receive from the mailbox");


	tsk_exit();
}

void task3(void)
{
	int test_id = g_test_id;
	task_t tid = tsk_gettid();

	int ret_val = 0;
	int sub_result = 0;
	U8 *p_index = &(g_ae_xtest.index);

	// Update the exec sequence | should run after task2
	update_exec_seq(test_id, tid);
	
	// Create a mailbox of 80 bytes
	ret_val = mbx_create(BUF_LEN);
	sub_result = ret_val >= 0 ? 1 : 0;
	update_tests(test_id, p_index, sub_result, "task3: creating a mailbox");


	U8 *buf = &g_buf3[0]; // Use the buffer for this task

	// After this we should go back to task 2
	init_message(buf, tid, "MFMALEX", 8);
	ret_val = send_msg(g_tids[2], (void *)buf);
	update_tests_rtx(test_id, p_index, ret_val, "task3: sending a message to task 2");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);

	ret_val = recv_msg(g_buf3, BUF_LEN);

	// Validate that the message is what we expect
	ret_val = validate_message(g_buf3, g_tids[1], DEFAULT, "888888888888888888888888888888888888888888888888888888888888888");
	update_tests_rtx(test_id, p_index, ret_val, "task3:  message from mailbox is the expected value");
	exit_on_fail(ret_val);

	// Update the exec sequence | should run after task1
	update_exec_seq(test_id, tid);


	ret_val = recv_msg(g_buf3, BUF_LEN);
	update_tests_rtx(test_id, p_index, ret_val, "task3:  receive from the mailbox");
	exit_on_fail(ret_val);

	ret_val = validate_message(g_buf3, g_tids[2], DEFAULT, "3333333333");
	update_tests_rtx(test_id, p_index, ret_val, "task3:  message from mailbox is the expected value");
	exit_on_fail(ret_val);

	// After this we should be blocked on receiving the message from task 1
	ret_val = recv_msg(g_buf3, BUF_LEN);

	ret_val = validate_message(g_buf3, g_tids[1], DEFAULT, "3L-1");
	update_tests_rtx(test_id, p_index, ret_val, "task3:  message from mailbox is the expected value");
	exit_on_fail(ret_val);

	// Exit the task
	tsk_exit();
}
