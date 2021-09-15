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
 * @file        priv_tasks.c
 * @brief       Two privileged tasks: priv_task1 and priv_task2
 *
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @note        Each task is in an infinite loop. These Tasks never terminate.
 *
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
 
#define     BUF_LEN         128
#define     MY_MSG_TYPE     100     // some customized message type, better move it to common_ext.h

#define 		NUM_TESTS   		1       // number of tests
#define 		NUM_TASKS   		5       // number of tasks being created
#define     NUM_INIT_TASKS  1       // number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
const char   PREFIX[]      = "G10-TS1";
const char   PREFIX_LOG[]  = "G10-TS1-LOG";
const char   PREFIX_LOG2[] = "G10-TS1-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
task_t       global_tid[NUM_TASKS];
int          cnt = 0;

//GENERAL PURPOSE BUFFERS
U8 g_buf1[CON_MBX_SIZE];	//send from task1
U8 g_buf2[CON_MBX_SIZE];	//send from task2
U8 g_buf3[CON_MBX_SIZE];	//send from task3
U8 g_buf4[CON_MBX_SIZE];	//send from task4
U8 g_buf5[CON_MBX_SIZE];	//recv: all above

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

void task4(void);

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

void set_ae_tasks(TASK_INIT *tasks, int num)
{
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;
    tasks[0].priv = 1;
    tasks[0].ptask = &priv_task1;

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
    g_tsk_cases[test_id].p_ae_case->num_bits = 7;	//NUM TEST CASES
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 29; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 28;
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
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


//START HELPER FUNCTIONS

// Message - string to be copied in
// Length - length of the string in bytes
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

// TEMP - prints out data field
void print_message(U8 *buf)
{
	RTX_MSG_HDR *ptr = (void *)buf;
	U32 length = ptr->length;

	// Get the length of the data
	int data_length = length - MSG_HDR_SIZE;

	U8 *data_ptr = buf + MSG_HDR_SIZE;

	for (int i = 0; i < data_length; i++)
	{
		printf("%c",data_ptr[i]);
	}
	printf("->");
}

//END HELPER FUNCTIONS


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
		
		//create mailbox for priv_task1
		ret_val0 = mbx_create(CON_MBX_SIZE);
		
		//set prio of priv_task1 to LOWEST to allow pre-emption
		ret_val1 = tsk_set_prio(tid, LOWEST);
			
		//TEST CASE 1 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 5 && ret_val0 == tid && !ret_val1){
				printf("TEST CASE 1: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 1: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
		//create task1
		ret_val0 = tsk_create(&global_tid[1], &task1, LOW, 0x200);
		
		//create task2
		ret_val1 = tsk_create(&global_tid[2], &task2, MEDIUM, 0x200);
		
		//create task3
		ret_val2 = tsk_create(&global_tid[3], &task3, HIGH, 0x200);
		
		//create task4
//		ret_val3 = tsk_create(&global_tid[4], &task4, MEDIUM, 0x200);
		
		//TEST CASE 5 - created tasks successfully
		if(!ret_val0 && !ret_val1 && !ret_val2/* && !ret_val3*/){
				printf("TEST CASE 5: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 5: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
		//used to check correct message order
		ret_val=1;
		
		//receive all messages in order
		for(int i=0; i<11; i++){	//CHECK
				recv_msg(g_buf5, CON_MBX_SIZE);
			
				if(i==0 || i==3 || i==4 || i==6 || i==8 || i==10){	//expected queue positions for task1 sent message
						if(validate_message(g_buf5, 2, DEFAULT, "1")){
								ret_val=0;
						}
				}else if(i==1 || i==5 || i==9){	//expected queue positions for task2 sent message
						if(validate_message(g_buf5, 3, DEFAULT, "2")){
								ret_val=0;
						}					
				}else if(i==2 || i==7){	//expected queue positions for task3 sent message
						if(validate_message(g_buf5, 4, DEFAULT, "3")){
								ret_val=0;
						}					
				}/*else{	//expected queue positions for task4 sent message
						if(validate_message(g_buf5, 5, DEFAULT, "4")){
								ret_val=0;
						}			
				}
*/			
				//test print, verified in TEST CASE 8 (below)
				print_message(g_buf5);
				//
		}
		printf("\n");
		
		//TEST CASE 6 - verify messages come in expected order
		if(ret_val){
				printf("TEST CASE 6: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 6: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;		

		//TEST CASE 7 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,2,3,4,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 8){	//real-time tasks still SUSPENDED (not DORMANT)
				printf("TEST CASE 7: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 7: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
		
//END
		
		test_exit();
}

void task1(void)
{
	  task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
	
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;		
	
		int ret_val;

//START
	
		printf("=> START: task1() w/ tid=%d\n", tid);
	
		//elevate task1 to real-time task
		TIMEVAL tv;
    tv.sec  = 0;		//CHECK
    tv.usec = 100000;		//CHECK
		ret_val = rt_tsk_set(&tv);

		//TEST CASE 2 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,2,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 6 && !ret_val){
				printf("TEST CASE 2: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 2: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

		//set-up message
		U8 *buf1 = &g_buf1[0];
		init_message_DEFAULT(buf1, tid, "1", 1);
	
		for(int i=0; i<6; i++){			
				//send message
				send_msg(1, (void *)buf1);

				//suspend task until next run
				rt_tsk_susp();
		}

//END
	
		tsk_exit();
}

void task2(void)
{
	  task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;

    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;		
	
		int ret_val;

//START
	
		printf("=> START: task2() w/ tid=%d\n", tid);
	
		//elevate task2 to real-time task
		TIMEVAL tv;
    tv.sec  = 0;		//CHECK
    tv.usec = 200000;		//CHECK
		ret_val = rt_tsk_set(&tv);

		//TEST CASE 3 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,2,3,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 7 && !ret_val){
				printf("TEST CASE 3: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 3: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;

		//set-up message
		U8 *buf2 = &g_buf2[0];
		init_message_DEFAULT(buf2, tid, "2", 1);
	
		for(int i=0; i<3; i++){
				//send message
				send_msg(1, (void *)buf2);
			
				//suspend task until next run
				rt_tsk_susp();
		}

//END
	
		tsk_exit();
}

void task3(void)
{
	  task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
	
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;		
	
		int ret_val;
	
//START
	
		printf("=> START: task3() w/ tid=%d\n", tid);
	
		//elevate task3 to real-time task
		TIMEVAL tv;
    tv.sec  = 0;		//CHECK
    tv.usec = 300000;	//CHECK
		ret_val = rt_tsk_set(&tv);

		//TEST CASE 4 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,2,3,4,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 8 && !ret_val){
				printf("TEST CASE 4: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 4: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
	
		//set-up message
		U8 *buf3 = &g_buf3[0];
		init_message_DEFAULT(buf3, tid, "3", 1);
	
		for(int i=0; i<2; i++){
				//send message
				send_msg(1, (void *)buf3);
			
				//suspend task until next run
				rt_tsk_susp();
		}

//END
	
		tsk_exit();
}
/*
void task4(void)
{
	  task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
	
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;		

//START
	
		printf("=> START: task4() w/ tid=%d\n", tid);

		//TEST CASE 5 - check mbx_ls (tid=1,13,14,15) and tsk_ls (tid=0,1,2,3,4,5,13,14,15)
		if(mbx_ls(g_buf5, 9) == 4 && tsk_ls(g_buf5, 9) == 9){
				printf("TEST CASE 5: PASS\n");
				sub_result = 1;
				process_sub_result(test_id, *p_index, sub_result);
		}else{
				printf("TEST CASE 5: FAIL\n");
				sub_result = 0;
		}
		(*p_index)++;
	
		//set-up message
		U8 *buf4 = &g_buf4[0];
		init_message_DEFAULT(buf4, tid, "4", 1);
	
		for(int i=0; i<20; i++){	//CHECK
				//send message
				send_msg(1, (void *)buf4);
			
				//test, artifical delay
				for ( int x = 0; x < DELAY; x++); // some artifical delay
		}

//END
	
		tsk_exit();
}
*/
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
