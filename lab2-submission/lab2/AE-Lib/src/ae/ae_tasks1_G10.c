#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

    
#define NUM_TESTS   4           // number of tests
#define NUM_DUMMY_TASKS MAX_TASKS - 3
 
const char   PREFIX[]      = "G10-TS1";
const char   PREFIX_LOG[]  = "G10-TS1-LOG";
const char   PREFIX_LOG2[] = "G10-TS1-LOG2";

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
    tasks[1].prio = LOWEST;
    
    init_ae_tsk_test();
}


void dummy_task() {
    task_t tid = tsk_gettid();
    int    test_id = g_dummy_test_id;
    
    printf("%s: TID = %d, dummy: entering\r\n", PREFIX_LOG2, tid);
    update_exec_seq(test_id, tid);

    printf("%s: TID = %d, dummy: exiting \r\n", PREFIX_LOG2, tid);
    tsk_exit();
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
    g_tsk_cases[test_id].p_ae_case->num_bits = NUM_DUMMY_TASKS; 
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = NUM_DUMMY_TASKS + 2; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = NUM_DUMMY_TASKS + 1;
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void gen_req1(int test_id)
{
    //bits[0:3] pos check, bits[4:9] for exec order check
    g_tsk_cases[test_id].p_ae_case->num_bits = NUM_DUMMY_TASKS + 2;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 0;       // N/A for this test
    g_tsk_cases[test_id].pos_expt = 0;  // N/A for this test
    
    update_ae_xtest(test_id);
}


int test0_start(int test_id)
{

    int ret_val = 10;

    task_t tids[NUM_DUMMY_TASKS];

    gen_req0(test_id);

    U8 *p_index = &(g_ae_xtest.index);
    int sub_result = 0;

    *p_index = 0;

    strcpy(g_ae_xtest.msg, "creating new task");
    printf("creating a MEDIUM prio task that runs dummy task\r\n");
    for (int i = 0; i < NUM_DUMMY_TASKS; i++)
    {
        ret_val = tsk_create(&tids[i], &dummy_task, MEDIUM, 0x200); /*create a user task */
        sub_result = (ret_val == RTX_OK) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);

        if (ret_val != RTX_OK)
        {
            sub_result = 0;
            test_exit();
        }
        
        (*p_index)++;
    }


    task_t *p_seq_expt = g_tsk_cases[test_id].seq_expt;

    // First task should be the current task
    p_seq_expt[0] = tsk_gettid();

    for (int i = 0; i < NUM_DUMMY_TASKS; i++) {
        p_seq_expt[i + 1] = tids[i];
    }
    
    return RTX_OK;
}


/**
 * @brief   task yield exec order test
 * @param   test_id, the test function ID 
 * @param   ID of the test function that logs the testing data
 * @note    usually test data is logged by the same test function,
 *          but some time, we may have multiple tests to check the same test data
 *          logged by a particular test function
 */
void test1_start(int test_id, int test_id_data)
{  
    gen_req1(test_id);
    
    U8      pos         = g_tsk_cases[test_id_data].pos;
    U8      pos_expt    = g_tsk_cases[test_id_data].pos_expt;
    task_t  *p_seq      = g_tsk_cases[test_id_data].seq;
    task_t  *p_seq_expt = g_tsk_cases[test_id_data].seq_expt;
       
    U8      *p_index    = &(g_ae_xtest.index);
    int     sub_result  = 0;
    
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



    int diff = pos - pos_expt;
    // test 1-[0]
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "checking execution shortfalls");
    sub_result = (diff < 0) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);
    
    for ( int i = 0; i < pos_expt; i ++ ) {
        (*p_index)++;
        sprintf(g_ae_xtest.msg, "checking execution sequence @ %d", i);
        sub_result = (p_seq[i] == p_seq_expt[i]) ? 1 : 0;
        process_sub_result(test_id, *p_index, sub_result);
    }
}

void run_test(int test_id) {
    task_t  tid         = tsk_gettid();
    g_dummy_test_id = test_id;


    test0_start(test_id);
    update_exec_seq(test_id, tid);
    tsk_set_prio(tid, LOW);

    test1_start(test_id + 1, test_id);
}

void priv_task1(void) {
    printf("Test trying to allocate a lot of tasks over and over again\r\n");
    printf("NIL\r\n");
    task_t  tid         = tsk_gettid();
    int     test_id     = 0;


    for (int i = 0; i < NUM_TESTS / 2; i++) {
        tsk_set_prio(tid, HIGH);
        run_test(test_id + (i * 2));
    }

    test_exit();
}

// We should never get to this point
void task1(void)
{
    test_exit();
} 
