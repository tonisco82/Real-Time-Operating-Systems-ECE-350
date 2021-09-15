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
 * @file        ae_tasks1_G10.c
 * @brief       Lab2 Testing Suite 1 Group 10
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
    
#define NUM_TESTS  			2       // number of tests
#define NUM_INIT_TASKS  1       // number of tasks during initialization
#define NUM_TASKS				3
#define BUF_LEN         128     // receiver buffer length
#define MY_MSG_TYPE     100     // some customized message type
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
 /*
	*==========================================================================
										INITIAL STATE OF PRIORITY READY QUEUE
	[HIGH]
	[MEDIUM] -> priv_task1
	[LOW] -> task2 -> task1
	[LOWEST]
	[NULL TASK PRIO]
	
										FINAL STATE OF PRIORITY READY QUEUE
	[HIGH]
	[MEDIUM] -> priv_task1
	[LOW] -> task1 -> task2
	[LOWEST]
	[NULL TASK PRIO]

	*==========================================================================
 */
const char   PREFIX[]      = "G10-TS1";
const char   PREFIX_LOG[]  = "G10-TS1-LOG";
const char   PREFIX_LOG2[] = "G10-TS1-LOG2";
TASK_INIT   g_init_tasks[NUM_INIT_TASKS];

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];


task_t order_of_exec[9]; // Array to hold order of execution.
task_t expected_order_of_exec[9] = {1,3,1,3,2,3,1,3,2};
int order_cnter = 0;

task_t       global_tid[NUM_TASKS];
int cnt = 0;

int total_tests = 12;
int passed_tests = 0;


//GENERAL PURPOSE BUFFERS (for recv)
U8 task1_buf[CON_MBX_SIZE];
U8 task2_buf[CON_MBX_SIZE];
U8 priv_task1_buf[CON_MBX_SIZE];

task_t g_tasks[MAX_TASKS];
task_t g_tids[MAX_TASKS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = MEDIUM;
        tasks[i].priv = 1;
    }

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

int update_exec_order(task_t tid)
{
		order_of_exec[order_cnter] = tid;
		order_cnter++;
		return RTX_OK;
}

void priv_task1(void)
{
    task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    U8  *p_index    = &(g_ae_xtest.index);
    int ret_val;
	
		printf("=>=================================\n");
		printf("%s: => START: priv_task1()\n", PREFIX_LOG);
		printf("%s: priv_task1: has TID %d\n", PREFIX_LOG, tid);
		printf("=>=================================\n");
		update_exec_order(tid);
	
		//TEST 1
    printf("%s: priv_task1: creating a mailbox of size %u Bytes\n", PREFIX_LOG, CON_MBX_SIZE);
    ret_val = mbx_create(CON_MBX_SIZE);  // create a mailbox for itself
    if (ret_val >= 0) {
				printf("%s: priv_task1: Successfully created mailbox.\n", PREFIX_LOG);
				printf("%s: PASSED TEST 1 !!!\n", PREFIX_LOG);
				passed_tests++;
    }
		
		//TEST 2
		printf("%s: priv_task1: creating task2... prio: LOW\n", PREFIX_LOG);
		task_t tid_tsk2;
    ret_val = tsk_create(&tid_tsk2, &task2, LOW, 0x200);
		if (ret_val == 0) {
				printf("%s: priv_task1: Successfully created task2.\n", PREFIX_LOG);
				printf("%s: PASSED TEST 2 !!!\n", PREFIX_LOG);
				passed_tests++;
    }
			
		//TEST 3
    (*p_index)++;
    printf("%s: priv_task1: creating task1... prio: HIGH\n", PREFIX_LOG);
		task_t tid_tsk1;
    ret_val = tsk_create(&tid_tsk1, &task1, HIGH, 0x200);  /*create a user task */
    if (ret_val == 0) {
				printf("%s: priv_task1: Successfully created task1.\n", PREFIX_LOG);
				printf("%s: PASSED TEST 3 !!!\n", PREFIX_LOG);
				passed_tests++;
    }
		// Task 1 should now preempt.
		
		// Back from task1 preemption.
		printf("=>=================================\n");
		printf("%s: => Re-entering: priv_task1()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		// Create message in buffer to send to task1
		// TEST 7
		printf("%s: priv_task1: create message to send to task1 (which has a full mailbox)...\n", PREFIX_LOG);
		RTX_MSG_HDR *buf = mem_alloc(sizeof(RTX_MSG_HDR));   
    buf->length = sizeof(RTX_MSG_HDR);
    buf->type = DEFAULT;
    buf->sender_tid = tid;
    ret_val = send_msg(tid_tsk1, buf);
		if (ret_val == 0) {
			printf("%s: priv_task1: Message unsuccessfully sent (CORRECT) to task1 as its mailbox is full. Now in BLK_SEND state\n", PREFIX_LOG);
			printf("%s: PASSED TEST 7 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		//**************************************************************
		// priv_task1 should now be in BLK_SEND state.
		//**************************************************************
		
		// Scheduler should reschedule task1 to run here.
		// task1 running...
		
		// Under this comment priv_task1 is running again after task1 emptied its mailbox to unblock priv_task1.
		// Send a message to task2 to unblock it.
		
		// Now reentering priv_task1 after task1 clears space in its mailbox.
		printf("=>=================================\n");
		printf("%s: => Re-entering: priv_task1()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		// TEST 11
		RTX_MSG_HDR *buf2 = mem_alloc(sizeof(RTX_MSG_HDR));   
    buf2->length = sizeof(RTX_MSG_HDR);
    buf2->type = DEFAULT;
    buf2->sender_tid = tid;
    ret_val = send_msg(tid_tsk2, buf2);
		if (ret_val == 0) {
			printf("%s: priv_task1: Message successfully sent to task2 to unblock it. TEST PASSED\n", PREFIX_LOG);
			printf("%s: PASSED TEST 11 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		// task2 should be unblocked and READY and added to the BACK of its priority ready queue (LOW).
		
		// READY QUEUE LOOKS LIKE THIS
		// State of priority ready queue should look like this:
		/*
		*	[HIGH]
		*	[MEDIUM] -> priv_task1
		*	[LOW] -> task1 -> task2
		*	[LOWEST]
		*/
		
		tsk_exit();
}

void task1(void)
{
		task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int ret_val;
	
		printf("=>=================================\n");
		printf("%s: => START: task1\n", PREFIX_LOG);
		printf("%s: task1: has TID %d\n", PREFIX_LOG, tid);
		printf("=>=================================\n");
		update_exec_order(tid);
	
		// Need this task to have a full mailbox so that task2 will enter BLK_SEND state.
	
		// Create mailbox for this task.
		// Note: CON_MBX_SIZE is 0x80
		// TEST 4
		printf("%s: task1: creating a mailbox of size %u Bytes\n", PREFIX_LOG, CON_MBX_SIZE); //128 bytes
		ret_val = mbx_create(CON_MBX_SIZE); // create a mailbox for itself
		if (ret_val >= 0) {
			printf("%s: task1: Mailbox successfully created. Free space in mailbox is %d Bytes. TEST PASSED\n", PREFIX_LOG, mbx_get(tid));
			printf("%s: PASSED TEST 4 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		// Send message to itself that will fill entire mailbox (size = 0x80)
		// TEST 5
    RTX_MSG_HDR *buf = mem_alloc(0x80);   
    buf->length = 0x80;
    buf->type = DEFAULT;
    buf->sender_tid = tid;
    ret_val = send_msg(tid, buf);
		if (ret_val == 0) {
			printf("%s: task1: Message successfully sent to itself.\n", PREFIX_LOG);
			printf("%s: PASSED TEST 5 !!!\n", PREFIX_LOG);
			passed_tests++;
			//Should be 0 free space in mailbox.
		}
		
		// TEST 6
		// TEST TO SEE IF MAILBOX IS FULL.
		// Check if mailbox is full (It should be full)
		ret_val = mbx_get(tid);
		if (ret_val == 0) {
			printf("%s: task1: mailbox is full. Free space in mailbox is %d Bytes.\n", PREFIX_LOG, ret_val); //Should be 0 free space in mailbox.
			printf("%s: PASSED TEST 6 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		printf("%s: task1: changing prio -> LOW\n", PREFIX_LOG);
		printf("%s: task1: because prio is now LOW, should reschedule to priv_task1 again...\n", PREFIX_LOG);
		tsk_set_prio(tid, LOW);
		
		// Now scheduler should reschedule priv_task1 to run again.
		
		// Re-enter task1 after priv_task1 goes to BLK_SEND state
		printf("=>=================================\n");
		printf("%s: => Re-entering: task1()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		// yeild for task2 to run.
		tsk_yield();
		
		// Under this comment is when priv_task1 and task2 both in BLK_SEND and BLK_RECV respectively.
		// Thus, now task1 is run again.
		printf("=>=================================\n");
		printf("%s: => Re-entering: task1()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		// TEST 10
		// Make task1 have space to receive message that priv_task1 tried to send.
		ret_val = recv_msg(task1_buf, 0x80);
		if (ret_val == 0) {
			printf("%s: task1: received message to clear mailbox.\n", PREFIX_LOG);
			printf("%s: PASSED TEST 10 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		// Now that task1 has space to receive the message that priv_task1 tried to send,
		// priv_task1 should now be in the READY state and pre-empt task1.
		// task1 should be added to the FRONT of its priority queue (LOW).
		// priv_task1 should be added to the BACK of its priority queue (MEDIUM).
		
		// READY QUEUE LOOKS LIKE THIS
		// State of priority ready queue should look like this:
		/*
		*	[HIGH]
		*	[MEDIUM] -> priv_task1
		*	[LOW] -> task1 -> task2
		*	[LOWEST]
		*/
	
		// priv_task1 should run now...
		
		// After priv_task1 calls tsk_exit()
		printf("=>=================================\n");
		printf("%s: => Re-entering: task1()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		tsk_exit();
	
}

void task2(void)
{
		task_t tid = tsk_gettid();
    global_tid[cnt] = tid;
    cnt++;
    int ret_val;
	
		printf("=>=================================\n");
		printf("%s: => START: task2\n", PREFIX_LOG);
		printf("%s: task2: has TID %d\n", PREFIX_LOG, tid);
		printf("=>=================================\n");
		update_exec_order(tid);
	
		// Make task2 call recv_msg() but it has an empty mailbox,
		// Thus, task2 will now be in the BLK_RECV state and the scheduler should reschedule to task1.
	
		// TEST 8
		// Create mailbox for this task.
		// TEST CAN CREATE MAILBOX.
		printf("%s: task2: creating a mailbox of size %u Bytes\n", PREFIX_LOG, CON_MBX_SIZE); //128 bytes
		ret_val = mbx_create(CON_MBX_SIZE); // create a mailbox for itself
		if (ret_val >= 0) {
			printf("%s: task2: Mailbox successfully created. Free space in mailbox is %d Bytes.\n", PREFIX_LOG, mbx_get(tid));
			printf("%s: PASSED TEST 8 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		// At this point mailbox is created but there are no messages in it.
		// Calling recv_msg() should now cause this task to go to BLK_RECV state.
		
		// TEST 9
		// TEST RECV_MSG WHEN MAILBOX EMPTY FAILS.
		ret_val = recv_msg(priv_task1_buf, CON_MBX_SIZE);
		if (ret_val == 0) {
			printf("%s: task2: recv_msg failed since maibox is empty.\n", PREFIX_LOG, mbx_get(tid));
			printf("%s: PASSED TEST 9 !!!\n", PREFIX_LOG);
			passed_tests++;
		}
		
		//**************************************************************
		// This task (task2) should now be in BLK_RECV state.
		//**************************************************************
		
		// Scheduler should now reschedule task1 to run.
		
		// After task1 calls tsk_exit()
		printf("=>=================================\n");
		printf("%s: => Re-entering: task2()\n", PREFIX_LOG);
		printf("=>=================================\n");
		update_exec_order(tid);
		
		// TEST 12
		// Check the order of execution is correct.
		// This proves that the scheduler functions correctly when tasks are in the BLK_SEND and BLK_RECV states.
		// This also shows that tasks properly enter and exit the BLK_SEND and BLK_RECV states.
		
		printf("%s: Expected order of execution...\n", PREFIX_LOG);
		for (int i = 0; i < 9; i++) {
				printf("%d -> ", expected_order_of_exec[i]);
		}
		printf("NULL\n");
		
		printf("%s: Actual order of execution...\n", PREFIX_LOG);
		for (int i = 0; i < 9; i++) {
				printf("%d -> ", order_of_exec[i]);
		}
		printf("NULL\n");
		
		int pass = 0;
		for (int i = 0; i < 9; i++) {
				if (order_of_exec[i] != expected_order_of_exec[i]) {
						printf("%s: Incorrect order of execution...\n", PREFIX_LOG);
						printf("%s: TEST 12 FAILED !!!\n", PREFIX_LOG);
						pass = 0;
						break;
				} else {
						pass = 1;
				}
		}
		if (pass == 1) {
				printf("%s: PASSED TEST 12 !!!\n", PREFIX_LOG);
				passed_tests++;
		}
		
		printf("====================================\n");
		printf("%s: TOTAL PASSED TESTS: %d/12\n", PREFIX_LOG, passed_tests);
		printf("%s: TOTAL FAILED TESTS: %d/12\n", PREFIX_LOG, total_tests-passed_tests);
		printf("====================================\n");
		
		printf("%s: END\r\n", PREFIX);
		ae_exit();
	
}
