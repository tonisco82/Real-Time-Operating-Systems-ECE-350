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
    
#define NUM_TESTS   		1          // number of tests
#define NUM_TASKS   		3           // number of tasks being created
#define NUM_INIT_TASKS 	2						// number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
//declare task1 and task2 since used in earlier tasks 
void task1(void);
void task2(void);
 
const char   PREFIX[]      = "G10-TS4";
const char   PREFIX_LOG[]  = "G10-TS4-LOG";
const char   PREFIX_LOG2[] = "G10-TS4-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
task_t       global_tid[NUM_TASKS];
int          cnt = 0;

//GENERAL PURPOSE BUFFERS (for recv)
U8 g_buf1[CON_MBX_SIZE];	//register command W to task1
U8 g_buf2[CON_MBX_SIZE];	//register command X to task1
U8 g_buf3[CON_MBX_SIZE];	//register command W to task2

U8 g_buf4[CON_MBX_SIZE];	//send register command %W first time (to task2)
U8 g_buf4a[CON_MBX_SIZE];	//send register command %W first time (to task2)
U8 g_buf4b[CON_MBX_SIZE];	//send register command %W first time (to task2)

U8 g_buf5[CON_MBX_SIZE];	//send register command %X first time (to task1)
U8 g_buf5a[CON_MBX_SIZE];	//send register command %X first time (to task1)
U8 g_buf5b[CON_MBX_SIZE];	//send register command %X first time (to task1)

U8 g_buf6[CON_MBX_SIZE];	//re-register command W to task2

U8 g_buf7[CON_MBX_SIZE];	//send register command %W second time (to task2)
U8 g_buf7a[CON_MBX_SIZE];	//send register command %W second time (to task2)
U8 g_buf7b[CON_MBX_SIZE];	//send register command %W second time (to task2)

U8 g_buf8[CON_MBX_SIZE];	//recv: everything

U8 g_buf9[CON_MBX_SIZE];	//send register command %X second time (to NOBODY)
U8 g_buf9a[CON_MBX_SIZE];	//send register command %X second time (to NOBODY)
U8 g_buf9b[CON_MBX_SIZE];	//send register command %X second time (to NOBODY)

U8 g_buf10[CON_MBX_SIZE];	//send register command A with no % (to NOBODY)
U8 g_buf10a[CON_MBX_SIZE];	//send register command A with no % (to NOBODY)

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

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
    g_tsk_cases[test_id].p_ae_case->num_bits = 11;  
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
void init_message_KCD_REG(U8 *buf, int tid, const char *message, int length)
{
	size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
	struct rtx_msg_hdr *ptr = (void *)buf;
	ptr->length = msg_hdr_size + length; // set the message length
	ptr->type = KCD_REG;			// set message type
	ptr->sender_tid = tid;			// set sender id

	U8 *temp_buf = buf + msg_hdr_size;
	strcpy((char *)temp_buf, message);
	return;
}

void init_message_KCD_CMD(U8 *buf, int tid, const char *message, int length)
{
	size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
	struct rtx_msg_hdr *ptr = (void *)buf;
	ptr->length = msg_hdr_size + length; // set the message length
	ptr->type = KCD_CMD;			// set message type
	ptr->sender_tid = tid;			// set sender id

	U8 *temp_buf = buf + msg_hdr_size;
	strcpy((char *)temp_buf, message);
	return;
}

void init_message_KEY_IN(U8 *buf, int tid, const char *message, int length)
{
	size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
	struct rtx_msg_hdr *ptr = (void *)buf;
	ptr->length = msg_hdr_size + length; // set the message length
	ptr->type = KEY_IN;			// set message type
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
	
		int ret_val0, ret_val1;
    
    if (tid == 1)
    {
        printf("%s: TID = %d, priv_task1: starting test0\r\n", PREFIX_LOG2, tid);
        strcpy(g_ae_xtest.msg, "task being created");
        test0_start(test_id);
    }

//START
		
		printf("=> START: priv_task1()\n");

//	update_exec_seq(test_id, tid);		(NEEDED?)
		
		//change to lowest so preemption occurs
		tsk_set_prio(tid, LOWEST);
		
		
//SPLIT HERE from TS2

		
		//task1, task2 will recv the same command message from priv_task1, task1 will register
		
		//create task1
		ret_val0 = tsk_create(&global_tid[1], &task1, MEDIUM, 0x200); 

		//create task2
		ret_val1 = tsk_create(&global_tid[1], &task2, HIGH, 0x200);
		
		//TEST CASE 3 - created tasks successfully
		if(!ret_val0 && !ret_val1){	//0x80 - 0x20 = 0x60
				printf("TEST CASE 3: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 3: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
		//send command id %W (next task2 will run up to tsk_exit, back here as task1 blocked)
		U8 *buf4 = &g_buf4[0];	//send register command %W first time
		init_message_KEY_IN(buf4, tid, "%", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf4);
		
		U8 *buf4a = &g_buf4a[0];	//send register command %W first time
		init_message_KEY_IN(buf4a, tid, "W", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf4a);		

		U8 *buf4b = &g_buf4b[0];	//send register command %W first time
		init_message_KEY_IN(buf4b, tid, "\r", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf4b);
		
		//not currently unblocking task2
		
		//TEST CASE 7 - check mbx_ls and tsk_ls
		if(mbx_ls(g_buf8, 6) == 3 && tsk_ls(g_buf8, 6) == 6){					//CHECK
				printf("TEST CASE 7: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 7: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;		
		
		//send command id %X (next task1 will run up to tsk_exit, back here)
		U8 *buf5 = &g_buf5[0];	//send register command %X first time
		init_message_KEY_IN(buf5, tid, "%", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf5);
		
		U8 *buf5a = &g_buf5a[0];	//send register command %X first time
		init_message_KEY_IN(buf5a, tid, "X", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf5a);		
		
		U8 *buf5b = &g_buf5b[0];	//send register command %X first time
		init_message_KEY_IN(buf5b, tid, "\r", 1);	//6+3=9
		send_msg_nb(TID_KCD, (void *)buf5b);
		
		//TEST CASE 9 - check mbx_ls and tsk_ls
		if(mbx_ls(g_buf8, 6) == 2 && tsk_ls(g_buf8, 6) == 5){					//CHECK
				printf("TEST CASE 9: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 9: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;	
	
	//TO CONFIRM BELOW 2 TESTS BASED ON PIAZZA 435
		
		//send command id %X - should go error since task1 tsk_exit, no mailbox
		U8 *buf9 = &g_buf9[0];	//send register command %X second time
		init_message_KEY_IN(buf9, tid, "%", 1);
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf9);
		
		U8 *buf9a = &g_buf9a[0];	//send register command %X second time
		init_message_KEY_IN(buf9a, tid, "X", 1);
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf9a);

		U8 *buf9b = &g_buf9b[0];	//send register command %X second time
		init_message_KEY_IN(buf9b, tid, "\r", 1);
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf9b);
		
	//IN THIS CASE (send KCD_CMD using command id no longer linked to task):
		//CURRENT ASSUMPTION (piazza 435): send_msg passes, fills mailbox

		//TEST CASE 10 - should PASS + output to console: "Command not found"
		if(!ret_val0 && mbx_get(TID_KCD) == KCD_MBX_SIZE){	//preemption
				printf("TEST CASE 10: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 10: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;	
		
		//send command id that doesn't start with %
		U8 *buf10 = &g_buf10[0];
		init_message_KEY_IN(buf10, tid, "A", 1);
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf10);
		
		U8 *buf10a = &g_buf10a[0];
		init_message_KEY_IN(buf10a, tid, "\r", 1);
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf10a);
		
	//IN THIS CASE (send invalid KCD_CMD command):
		//CURRENT ASSUMPTION (piazza 435): send_msg passes, fills mailbox

		//TEST CASE 11 - should PASS + output to console: "Invalid command"
		if(!ret_val0 && mbx_get(TID_KCD) == KCD_MBX_SIZE){	//preemption
				printf("TEST CASE 11: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 11: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;			

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
	
		int ret_val0, ret_val1;
	
//START
	
		printf("=> START: task1() w/ tid=%d\n", tid);

//	update_exec_seq(test_id, tid);		(NEEDED?)	
	
		//create mailbox for task1
		mbx_create(CON_MBX_SIZE);	//CON_MBX_SIZE = 0x80 (can change size)
	
		//register command id %W
		U8 *buf1 = &g_buf1[0];
		init_message_KCD_REG(buf1, tid, "W", 1);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf1);

		//register command id %X
		U8 *buf2 = &g_buf2[0];
		init_message_KCD_REG(buf2, tid, "X", 1);	//6+2=8
		ret_val1 = send_msg(TID_KCD, (void *)buf2);

		//TEST CASE 1 - registered commands %W and %X correctly
		if(!ret_val0 && !ret_val1 && mbx_get(TID_KCD) == KCD_MBX_SIZE){	//preemption
				printf("TEST CASE 1: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 1: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

		//waiting for priv_task1 to send command identifier (registered)
		ret_val0 = recv_msg(g_buf8, CON_MBX_SIZE);
		
		//TEST CASE 8 - we should receive %X from priv_task1
		if(ret_val0 == 0 && !validate_message(g_buf8, 15, KCD_CMD, "%X")){
				printf("TEST CASE 8: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 8: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;		

//END

		tsk_exit();
}

void task2() {

		printf("=> START: task2()\n");	
	
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val0, ret_val1;
	
//START
	
		printf("=> START: task2() w/ tid=%d\n", tid);

//	update_exec_seq(test_id, tid);		(NEEDED?)	
	
		//create mailbox for task2
		mbx_create(CON_MBX_SIZE);	//CON_MBX_SIZE = 0x80 (can change size)
	
		//register command id %W
		U8 *buf3 = &g_buf3[0];
		init_message_KCD_REG(buf3, tid, "W", 2);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf3);

		//TEST CASE 2 - registered commands %W correctly
		if(!ret_val0 && mbx_get(TID_KCD) != KCD_MBX_SIZE){	//NO preemption
				printf("TEST CASE 2: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 2: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

		//waiting for priv_task1 to send command id %W (first time)
		ret_val0 = recv_msg(g_buf8, CON_MBX_SIZE);
		
		//TEST CASE 4 - we should receive the message since task2 most recently registered %W
		if(ret_val0 == 0 && !validate_message(g_buf8, 15, KCD_CMD, "%W")){
				printf("TEST CASE 4: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 4: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

		//re-register command id %W - should succeed
		U8 *buf6 = &g_buf6[0];
		init_message_KCD_REG(buf6, tid, "W", 2);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf6);

		//TEST CASE 5 - re-registering should work
		if(!ret_val0 && mbx_get(TID_KCD) != KCD_MBX_SIZE){	//NO preemption
				printf("TEST CASE 5: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 5: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;		
		
		//send KCD_CMD %W second time - should redirect back to itself
		U8 *buf7 = &g_buf7[0];
		init_message_KEY_IN(buf7, tid, "%", 1);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf7);	
		
		U8 *buf7a = &g_buf7a[0];
		init_message_KEY_IN(buf7a, tid, "W", 1);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf7a);	

		U8 *buf7b = &g_buf7b[0];
		init_message_KEY_IN(buf7b, tid, "\r", 1);	//6+2=8
		ret_val0 = send_msg(TID_KCD, (void *)buf7b);			
		
		//waiting for TID_KCD to send back command id %W (second time)
		ret_val1 = recv_msg(g_buf8, CON_MBX_SIZE);
		
		//TEST CASE 6 - re-registering should work - WILL TAKE UP MAILBOX SPACE?
		if(!ret_val0 && !ret_val1 && !validate_message(g_buf8, 15, KCD_CMD, "%W")){	//0x200-16-8-8?
				printf("TEST CASE 6: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 6: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;			

//END
		
		tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
