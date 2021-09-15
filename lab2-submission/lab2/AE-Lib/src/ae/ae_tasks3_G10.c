#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

    
#define NUM_TESTS   1           // number of tests
 
const char   PREFIX[]      = "G10-TS3";
const char   PREFIX_LOG[]  = "G10-TS3-LOG";
const char   PREFIX_LOG2[] = "G10-TS3-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];
int g_dummy_test_id = 0;

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH + i;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 1;
    tasks[0].ptask = &priv_task1;
    tasks[1].priv  = 0;
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
        g_tsk_cases[i].pos = 0;  
    }
    printf("%s: START\r\n", PREFIX);
}

// @TODO change this to reflect the actual test 
void gen_req0(int test_id)
{
    // bits[0:1] for two tsk_create, [2:5] for 4 tsk_yield return value checks
    g_tsk_cases[test_id].p_ae_case->num_bits = 18;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 0; // Does not matter for this task 
    g_tsk_cases[test_id].pos_expt = 0; // Does not matter for this task
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

int g_has_task_1_run = 0;

void priv_task1(void) {
    int test_id = 0;
    gen_req0(test_id);


    int sub_result = 0;
    task_t tid = tsk_gettid();
    U8 *p_index = &(g_ae_xtest.index);

    // We should receve a non-null pointer 
    void *ptr_1 = mem_alloc(RAM1_SIZE / 4);
    sub_result = ptr_1 != NULL ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;


    // There should only be 2 free blocks of memory
    int ret = mem_dump();
    sub_result = ret == 2 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;


    // It should deallocate with no issue
    ret = mem_dealloc(ptr_1);
    sub_result = ret == RTX_OK ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // It should have only one block (the entire memory)
    ret = mem_dump();
    sub_result = ret == 1 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // We should receve a non-null pointer 
    void *ptr_2 = mem_alloc(RAM1_SIZE / 4);
    sub_result = ptr_2 != NULL ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // We should now be at the same priority as the other task
    // Because this is considered involuntary, we should run immediately
    tsk_set_prio(tid, MEDIUM);

    // check to make sure we ran next and not task_1
    sub_result = g_has_task_1_run == 0 ? 1 : 0;
    process_sub_result(test_id,*p_index, sub_result);
    (*p_index)++;

    // Now we yield and we should switch to the next task
    tsk_yield();

    // Now task 1 should have run
    // Note: we still have 1K block of memory that belongs to us 
    //       task_1 should also have a block of memory that is 2K 
    sub_result = g_has_task_1_run == 1 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;



    // We should receve a non-null pointer 
    void *ptr_3 = mem_alloc(RAM1_SIZE / 4);
    sub_result = ptr_3 != NULL ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    
    // It should have NO memory blocks left 
    ret = mem_dump();
    sub_result = ret == 0 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;
    
    // It should deallocate with no issue
    ret = mem_dealloc(ptr_2);
    sub_result = ret == RTX_OK ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // It should have one memory block left 
    ret = mem_dump();
    sub_result = ret == 1 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    ret = mem_dump();
    (*p_index)++;

    // It should deallocate with no issue
    ret = mem_dealloc(ptr_3);
    sub_result = ret == RTX_OK ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    tsk_exit();
}

// We should never get to this point
void task1(void)
{
    int test_id = 0;
    int sub_result = 0;
    task_t tid = tsk_gettid();
    U8 *p_index = &(g_ae_xtest.index);

    // Set the flag so the private task can check if the prio switch worked correctly
    g_has_task_1_run = 1;


    // There should be 2 free blocks 
    int ret = mem_dump();
    sub_result = ret == 2 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // we should not be able to allocate the entire ram 
    void *ptr_1 = mem_alloc(RAM1_SIZE);
    sub_result = ptr_1 == NULL ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    
    // We should be able to allocate half of the ram 
    void *ptr_2 = mem_alloc(RAM1_SIZE / 2);
    sub_result = ptr_2 != NULL ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    // There should be 2 free blocks 
    ret = mem_dump();
    sub_result = ret == 1 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;


    // Move back to the other task
    tsk_yield();

    // It should deallocate with no issue
    ret = mem_dealloc(ptr_2);
    sub_result = ret == RTX_OK ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;


    // There should be 1 free blocks 
    ret = mem_dump();
    sub_result = ret == 1 ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
    (*p_index)++;

    test_exit();
} 
