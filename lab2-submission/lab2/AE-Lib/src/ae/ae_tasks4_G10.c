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
    
#define NUM_TESTS   1           // number of tests
#define NUM_TASKS   3           // number of tasks being created

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */
 
const char   PREFIX[]      = "G10-TS4";
const char   PREFIX_LOG[]  = "G10-TS4-LOG";
const char   PREFIX_LOG2[] = "G10-TS4-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
task_t       global_tid[NUM_TASKS];
int          count = 0;

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    tasks[0].u_stack_size = PROC_STACK_SIZE;
    tasks[0].prio = HIGH;
    tasks[0].priv = 1;
    tasks[0].ptask = &priv_task1;
    
    tasks[1].u_stack_size = PROC_STACK_SIZE;
    tasks[1].prio = MEDIUM;
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
    g_tsk_cases[test_id].p_ae_case->num_bits = 5;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 8; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 7;
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void end_test(int test_id)
{
    U8      pos         = g_tsk_cases[test_id].pos;
    U8      pos_expt    = g_tsk_cases[test_id].pos_expt;
    task_t  *p_seq      = g_tsk_cases[test_id].seq;
    task_t  *p_seq_expt = g_tsk_cases[test_id].seq_expt;

    p_seq_expt[0] = global_tid[0];
    p_seq_expt[1] = global_tid[1];
    p_seq_expt[2] = global_tid[1];
    p_seq_expt[3] = global_tid[2];
    p_seq_expt[4] = global_tid[1];
    p_seq_expt[5] = global_tid[2];
    p_seq_expt[6] = global_tid[0];
    
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

    test_exit();
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
 * @brief       a task that prints A, B on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

void priv_task1(void)
{
    task_t tid = tsk_gettid();
    global_tid[count] = tid;
    count++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    int ret_prio = 0;
    
    printf("%s: TID = %d, priv_task1: starting test0\r\n", PREFIX_LOG2, tid);
    strcpy(g_ae_xtest.msg, "task switch");
    test0_start(test_id);

    char out_char = 'A';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");
    printf("%s: TID = %d, priv_task1: Changing own priority\r\n", PREFIX_LOG2, tid);
    update_exec_seq(test_id, tid);
    ret_prio = tsk_set_prio(tid, LOWEST);
    update_exec_seq(test_id, tid);
    out_char = 'B';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");
    sub_result = (ret_prio == RTX_OK) ? 1 : 0;
    
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    RTX_TASK_INFO buffer;
    tsk_get(tid, &buffer);
    sub_result = 0;
    if (buffer.prio == LOWEST && buffer.priv == 1 && buffer.tid == tid && buffer.state == RUNNING)
    {
        sub_result = 1;
    }
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;
    
    
    end_test(0);
}

void task1() {
    task_t tid = tsk_gettid();
    global_tid[count] = tid;
    count++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;

    char out_char = 'C';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");
    printf("%s: TID = %d, task1: switching priority and yielding\r\n", PREFIX_LOG2, tid);
    update_exec_seq(test_id, tid);
    task_t new_tid;
    int ret_prio = tsk_set_prio(global_tid[0], LOW);
    tsk_create(&new_tid, &task2, MEDIUM, 0x200);
    update_exec_seq(test_id, tid);
    int ret_yield = tsk_yield();

    update_exec_seq(test_id, tid);
    out_char = 'D';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");

    sub_result = (ret_prio == RTX_ERR) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;
    sub_result = (ret_yield == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;
    tsk_exit();
}

void task2() {
    task_t tid = tsk_gettid();
    global_tid[count] = tid;
    count++;
    int test_id = 0;
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;

    char out_char = 'E';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");
    printf("%s: TID = %d, task1: switching priority\r\n", PREFIX_LOG2, tid);
    update_exec_seq(test_id, tid);
    
    int ret_prio = tsk_set_prio(global_tid[1], HIGH);

    update_exec_seq(test_id, tid);
    out_char = 'F';
    uart1_put_char(out_char);
    uart1_put_string("\r\n");

    sub_result = (ret_prio == RTX_OK) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;
    tsk_exit();
}



/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
