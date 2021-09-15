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

/**************************************************************************//**
 * @file        ae_tasks200.c
 * @brief       Lab2 public testing suite 200
 *
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 Jun
 * *
 *****************************************************************************/

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
    
#define NUM_TESTS   		1           // number of tests
#define NUM_TASKS   		5           // number of tasks being created
#define NUM_INIT_TASKS 	2						// number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
const char   PREFIX[]      = "G10-TS2";
const char   PREFIX_LOG[]  = "G10-TS2-LOG";
const char   PREFIX_LOG2[] = "G10-TS2-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
task_t       global_tid[NUM_TASKS];
int          cnt = 0;

//GENERAL PURPOSE BUFFERS
U8 g_buf1[CON_MBX_SIZE];	//send: priv_task1 to priv_task1
U8 g_buf2[CON_MBX_SIZE];	//send: task1 to priv_task1
U8 g_buf3[CON_MBX_SIZE];	//send: task2 to priv_task1
U8 g_buf4[CON_MBX_SIZE];	//send: task3 to priv_task1
U8 g_buf5[CON_MBX_SIZE];	//recv: all above

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

//task_t g_tids[MAX_TASKS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;
    tasks[0].priv = 1;
    tasks[0].ptask = &priv_task1;
    
    tasks[1].u_stack_size = PROC_STACK_SIZE;
    tasks[1].prio = LOWEST;
    tasks[1].priv = 0;
    tasks[1].ptask = &task1;
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ ) {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
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

void gen_req0(int test_id)
{
    // bits[0:1] for two tsk_create, [2:5] for 4 tsk_yield return value checks
    g_tsk_cases[test_id].p_ae_case->num_bits = 6;	//NUM TEST CASES
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 29; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 28;
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}


//next 2 test utility functions added from ae_tasks3_G10.c

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


int test0_start(int test_id)
{
    
    gen_req0(test_id);
    
    return RTX_OK;
}

int update_exec_seq(int test_id, task_t tid) 
{

    U8 len = g_tsk_cases[test_id].len;
    U8 *p_pos = &g_tsk_cases[test_id].pos;
    task_t *p_seq = g_tsk_cases[test_id].seq;
    p_seq[*p_pos] = tid;
    (*p_pos)++;
    (*p_pos) = (*p_pos)%len;  // preventing out of array bound
    return RTX_OK;
}

/**************************************************************************//**
 * @brief       a task that prints AAAAA, BBBBB on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

void priv_task1(void)
{
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val, ret_val0, ret_val1, ret_val2, ret_val3;
    
    if (tid == 1)
    {
        printf("%s: TID = %d, priv_task1: starting test0\r\n", PREFIX_LOG2, tid);
        strcpy(g_ae_xtest.msg, "task being created");
        test0_start(test_id);
    }

//START
		
		printf("=> START: priv_task1() w/ tid=%d\n", tid);

//	update_exec_seq(test_id, tid);		(NEEDED?)
		
		//change to lowest so preemption occurs
		tsk_set_prio(tid, LOWEST);
		
		//create mailbox for priv_task1
		mbx_create(CON_MBX_SIZE);	//CON_MBX_SIZE = 0x80 (can change size)

/*		
		//task sends message to itself (size = 0x6)
    RTX_MSG_HDR *buf0 = mem_alloc(sizeof(RTX_MSG_HDR));   
    buf0->length = sizeof(RTX_MSG_HDR);
    buf0->type = DEFAULT;
    buf0->sender_tid = tid;
    ret_val = send_msg_nb(tid, buf0);
*/
		U8 *buf = &g_buf1[0];
		init_message(buf, tid, "from priv_task1", 15);	//6+15=21
		ret_val = send_msg_nb(1, (void *)buf);

//TEST CASE 1 - send msg to self
		printf("size of priv_task1 mailbox (after buf0): %d\n", mbx_get(1));
		if(ret_val == 0 && mbx_get(1) == 107){	//128-21=107
				//pass test, since successfully sent to self
				printf("TEST CASE 1: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 1: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
	//task1, task2, task3 will all send message to priv_task1
		
		//create task1
		tsk_create(&global_tid[1], &task1, MEDIUM, 0x200);  		
		
		//create task2
		tsk_create(&global_tid[2], &task2, HIGH, 0x200);  		
		
		//create task3
		tsk_create(&global_tid[3], &task3, LOW, 0x200);  		
		
		//recv until both queue, backlog empty
		printf("Printing message data in order:\n");
		
		//priv_task1 to priv_task1
		ret_val = recv_msg(g_buf5, CON_MBX_SIZE);
		if(!ret_val && !validate_message(g_buf5, 1, DEFAULT, "from priv_task1")){
				ret_val0 = 0;
		}else{
				ret_val0 = 1;
		}

		//task1 to priv_task1
		ret_val = recv_msg(g_buf5, CON_MBX_SIZE);
		if(!ret_val && !validate_message(g_buf5, 3, DEFAULT, "from task1")){
				ret_val1 = 0;
		}else{
				ret_val1 = 1;

		}
		
		//task2 to priv_task1
		ret_val = recv_msg(g_buf5, CON_MBX_SIZE);
		if(!ret_val && !validate_message(g_buf5, 3, DEFAULT, "from task2-------------------------------------------------------------------------")){
				ret_val2 = 0;
		}else{
				ret_val2 = 1;
		}

		//task3 to priv_task1 (initially blocked)
		ret_val = recv_msg(g_buf5, CON_MBX_SIZE);
		if(!ret_val && !validate_message(g_buf5, 3, DEFAULT, "from task3")){
				ret_val3 = 0;
		}else{
				ret_val3 = 1;
		}
		
//TEST CASE 5 - all sent messages successfully received (full free space) + printed
		if(ret_val0 == 0 && ret_val1 == 0 && ret_val2 == 0 && ret_val3 == 0 && mbx_get(1) == 0x80){
				printf("TEST CASE 5: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 5: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
//TEST CASE 6 - task1, task2, and task3 (same reused DORMANT tid=3) should have mailboxes deallocated (tsk_exit)
		if(mbx_get(3) == -1){	//ASSUMPTION OF TIDS
				printf("TEST CASE 6: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 6: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		

//SPLIT HERE from TS4
		
		
//END

    test_exit();		//FINISHED TEST SUITE
}

void task1() {
	
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val;
	
//START

		printf("=> START: task1() w/ tid=%d\n", tid);
	
//	update_exec_seq(test_id, tid);		(NEEDED?)	
	
		//send message to priv_task1 - NO DATA
/*
    RTX_MSG_HDR *buf1 = mem_alloc(0x3d);   //half of remaining space in priv_task1 mailbox
    buf1->length = 0x3d;
    buf1->type = DEFAULT;
    buf1->sender_tid = tid;
    ret_val = send_msg(1, buf1);	//priv_task1 has tid = 1
*/
		U8 *buf = &g_buf2[0];
		init_message(buf, tid, "from task1", 11);	//6+11=17
		ret_val = send_msg(1, (void *)buf);

//TEST CASE 2 - send from task1 to priv_task1 (buffer now ~half)
		printf("size of priv_task1 mailbox (after buf1): %d\n", mbx_get(1));
		if(ret_val == 0 && mbx_get(1) == 90){	//107-17=90
				printf("TEST CASE 2: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 2: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
//END

    tsk_exit();
}

void task2() {

    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val;
	
//START

		printf("=> START: task2() w/ tid=%d\n", tid);	
	
//	update_exec_seq(test_id, tid);		(NEEDED?)	
	
		//send message to priv_task1 - NO DATA
/*
		RTX_MSG_HDR *buf2 = mem_alloc(0x3d);   //all of remaining space in priv_task1 mailbox
    buf2->length = 0x3d;
    buf2->type = DEFAULT;
    buf2->sender_tid = tid;
    ret_val = send_msg_nb(1, buf2);	//priv_task1 has tid = 1
*/
		U8 *buf = &g_buf3[0];
		init_message(buf, tid, "from task2-------------------------------------------------------------------------", 84);	//6+84=90
		ret_val = send_msg_nb(1, (void *)buf);

//TEST CASE 3 - send from task2 to priv_task1 (buffer now full)
		printf("size of priv_task1 mailbox (after buf2): %d\n", mbx_get(1));
		if(ret_val == 0 && mbx_get(1) == 0){	//90-90=0
				printf("TEST CASE 3: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 3: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;		

//END	
	
		tsk_exit();
}

void task3() {
	
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val;
	
//START
	
		printf("=> START: task3() w/ tid=%d\n", tid);

//	update_exec_seq(test_id, tid);		(NEEDED?)	
	
		//send message to priv_task1 - NO DATA
/*
    RTX_MSG_HDR *buf3 = mem_alloc(0x20);   //no more remaining space in priv_task1 mailbox
    buf3->length = 0x20;
    buf3->type = DEFAULT;
    buf3->sender_tid = tid;
    ret_val = send_msg_nb(1, buf3);	//priv_task1 has tid = 1
*/
		U8 *buf = &g_buf4[0];
		init_message(buf, tid, "from task3", 11);	//6+11=17
		ret_val = send_msg_nb(1, (void *)buf);

//TEST CASE 4 - non-blocking send from task3 to priv_task1 (buffer already full) - should FAIL
		printf("size of priv_task1 mailbox (after buf3): %d\n", mbx_get(1));
		if(ret_val == -1 || mbx_get(1) == 0){
				printf("TEST CASE 4: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 4: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
	
		//should be blocked since priv_task1 msg queue is full, put into backlog for now
		ret_val = send_msg(1, (void *)buf);
		
//END	
	
    tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
