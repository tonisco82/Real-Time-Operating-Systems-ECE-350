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
#define NUM_TASKS   		2           // number of tasks being created
#define NUM_INIT_TASKS 	2						// number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
//declare task1 and task2 since used in earlier tasks 
void task1(void);
void task2(void);
 
const char   PREFIX[]      = "G10-TS5";
const char   PREFIX_LOG[]  = "G10-TS5-LOG";
const char   PREFIX_LOG2[] = "G10-TS5-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
task_t       global_tid[NUM_TASKS];
int          cnt = 0;

//GENERAL PURPOSE BUFFERS (for recv)
U8 g_buf1[CON_MBX_SIZE];	//generic message with newline character from task1
U8 g_buf1a[CON_MBX_SIZE];	//generic message with newline character
U8 g_buf1b[CON_MBX_SIZE];	//generic message with newline character

U8 g_buf2[CON_MBX_SIZE];	//%LM command from task1
U8 g_buf2a[CON_MBX_SIZE];	//%LM command
U8 g_buf2b[CON_MBX_SIZE];	//%LM command
U8 g_buf2c[CON_MBX_SIZE];	//%LM command

U8 g_buf3[CON_MBX_SIZE];	//%LT command from task1
U8 g_buf3a[CON_MBX_SIZE];	//%LT command
U8 g_buf3b[CON_MBX_SIZE];	//%LT command
U8 g_buf3c[CON_MBX_SIZE];	//%LT command

U8 g_buf4[CON_MBX_SIZE];	//non-DISPLAY command (DEFAULT) from task2

U8 g_buf5[CON_MBX_SIZE];	//recv: all above

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
    g_tsk_cases[test_id].p_ae_case->num_bits = 4;  
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

void init_message_DEFAULT(U8 *buf, int tid, const char *message, int length)
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

void init_message_DISPLAY(U8 *buf, int tid, const char *message, int length)
{
	size_t msg_hdr_size = sizeof(struct rtx_msg_hdr);
	struct rtx_msg_hdr *ptr = (void *)buf;
	ptr->length = msg_hdr_size + length; // set the message length
	ptr->type = DISPLAY;			// set message type
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
//    U8      *p_index    = &(g_ae_xtest.index);
//    int     sub_result  = 0;
	
//		int ret_val;
    
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

		//create task1 - MEDIUM
		tsk_create(&global_tid[1], &task1, MEDIUM, 0x200);

		//create task2 - HIGH
		tsk_create(&global_tid[2], &task2, HIGH, 0x200);		
		
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


//SEND T (should print invalid command to console)	
		//send generic message string with newline character - manual test, simulates interrupt handler function, check console
		U8 *buf1 = &g_buf1[0];
		init_message_KEY_IN(buf1, tid, "T", 1);       //CHECK LENGTH	//to change to individual
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf1);
		
		U8 *buf1a = &g_buf1a[0];
		init_message_KEY_IN(buf1a, tid, "\r", 1);       //CHECK LENGTH	//to change to individual
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf1a);

    //TEST CASE 1 - check generic message sent - MANUALLY CHECK CONSOLE FOR "INVALID COMMAND"
    if(!ret_val0 && mbx_get(TID_KCD) == KCD_MBX_SIZE){	//preemption
				printf("TEST CASE 1: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 1: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

//SEND %LT
		//send command id %LM (KCD should print all mbx_ls to console) - manual test, simulates interrupt handler function, check console
		U8 *buf2 = &g_buf2[0];
		init_message_KEY_IN(buf2, tid, "%", 1);	//CHECK LENGTH
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf2);
		//next characters individually
		U8 *buf2a = &g_buf2a[0];
		init_message_KEY_IN(buf2a, tid, "L", 1);	//CHECK LENGTH
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf2a);			
		//next characters individually
		U8 *buf2b = &g_buf2b[0];
		init_message_KEY_IN(buf2b, tid, "T", 1);	//CHECK LENGTH
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf2b);		
		//send "enter" to conclude message
		U8 *buf2c = &g_buf2c[0];
		init_message_KEY_IN(buf2c, tid, "\r", 1);	//CHECK LENGTH
		ret_val0 = send_msg_nb(TID_KCD, (void *)buf2c);
//	

//SEND %LM
		//send command id %LM (KCD should print all mbx_ls to console) - manual test, simulates interrupt handler function, check console
		U8 *buf3 = &g_buf3[0];
		init_message_KEY_IN(buf3, tid, "%", 1);	//CHECK LENGTH
		ret_val1 = send_msg_nb(TID_KCD, (void *)buf3);
		//next characters individually
		U8 *buf3a = &g_buf3a[0];
		init_message_KEY_IN(buf3a, tid, "L", 1);	//CHECK LENGTH
		ret_val1 = send_msg_nb(TID_KCD, (void *)buf3a);		
		//next characters individually
		U8 *buf3b = &g_buf3b[0];
		init_message_KEY_IN(buf3b, tid, "M", 1);	//CHECK LENGTH
		ret_val1 = send_msg_nb(TID_KCD, (void *)buf3b);
		//send "enter" to conclude message
		U8 *buf3c = &g_buf3c[0];
		init_message_KEY_IN(buf3c, tid, "\r", 1);	//CHECK LENGTH
		ret_val1 = send_msg_nb(TID_KCD, (void *)buf3c);
//	

    //TEST CASE 2 - check %LT and %LM messages sent
    if(!ret_val0 && !ret_val1 && mbx_get(TID_KCD) == KCD_MBX_SIZE){	//preemption
				printf("TEST CASE 2: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 2: FAIL\n");
				sub_result = 0;	
		}
		(*p_index)++;
		
		//send msg of DISPLAY type (w/ newline) to Console Display Task - manual test, check console
		U8 *buf4 = &g_buf4[0];
		init_message_DISPLAY(buf4, tid, "DISPLAY TYPE MSG\n", 18);       //CHECK LENGTH
		ret_val0 = send_msg_nb(TID_CON, (void *)buf4);

    //TEST CASE 3 - check DISPLAY message sent - TID_CON should display
    if(!ret_val0 && mbx_get(TID_CON) == CON_MBX_SIZE){	//preemption
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

void task2() {
	
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
	
		int ret_val0;
	
//START
	
		printf("=> START: task2() w/ tid=%d\n", tid);

//	update_exec_seq(test_id, tid);		(NEEDED?)	

		//send msg of not DISPLAY type to Console Display Task - manual test, check console
		U8 *buf4 = &g_buf4[0];
		init_message_DEFAULT(buf4, tid, "NOT DISPLAY TYPE - SHOULDN'T PRINT", 36);       //CHECK LENGTH
		ret_val0 = send_msg_nb(TID_CON, (void *)buf4);

    //TEST CASE 4 - check non-DISPLAY message sent - TID_CON should ignore (not display)
    if(!ret_val0 && mbx_get(TID_CON) != CON_MBX_SIZE){	//NO preemption
				printf("TEST CASE 4: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 4: FAIL\n");
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
