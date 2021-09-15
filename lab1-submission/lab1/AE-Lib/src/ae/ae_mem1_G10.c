/**
 * @brief Group 99 Lab1 Memory Test Suite 1 Template File
 */
#include "rtx.h"
#include "uart_polling.h"
#include "printf.h"

int test_mem(void) {
	
//START
	
		printf("G10-TS1: START\r\n");
	
		int x = 0;
		int y = 0;
	
		static void *p[4];
			
		for ( int i = 0; i < 4; i++ ) {
        p[i] = (void *)0x1;
    }
		
		p[0] = mem_alloc(0x2000);
		
	//TEST CASE 1: verify there are 2 available blocks left (0x2000, 0x4000)
		if (mem_dump() == 2) {
				x++;
			  printf("G10-TS1: TEST CASE 1 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 1 - FAIL\n");
		}		
		
		p[1] = mem_alloc(0x2000 - 2);
		p[2] = mem_alloc(0x2000);
		
	//TEST CASE 2: verify there is 1 available block left (0x2000), no blocks the same
		if (mem_dump() == 1 && p[0] != p[1] && p[1] != p[2]) {
				x++;
			  printf("G10-TS1: TEST CASE 2 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 2 - FAIL\n");
		}	
		
		p[3] = mem_alloc(0x2000 - 2);
		
	//TEST CASE 3: verify there are 0 available blocks left, no blocks the same
		if (mem_dump() == 0 && p[2] != p[3]) {
				x++;
			  printf("G10-TS1: TEST CASE 3 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 3 - FAIL\n");
		}	
		
		mem_dealloc(p[0]);
		p[0] = NULL;
		mem_dealloc(p[1]);
		p[1] = NULL;
		
	//TEST CASE 4: verify there is 1 available (coalesced) block left (0x4000)
		if (mem_dump() == 1) {
				x++;
			  printf("G10-TS1: TEST CASE 4 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 4 - FAIL\n");
		}			
	
		p[0] = mem_alloc(0x4000);
		
	//TEST CASE 5: verify that size 0x4000 (coalesced) block allocated correctly
		if (p[0] != NULL) {
				x++;
			  printf("G10-TS1: TEST CASE 5 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 5 - FAIL\n");
		}

	//TEST CASE 6: verify there are 0 available blocks left
		if (mem_dump() == 0) {
				x++;
			  printf("G10-TS1: TEST CASE 6 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 6 - FAIL\n");
		}	
		
		mem_dealloc(p[2]);
		p[2] = NULL;
		
		p[1] = mem_alloc(0x1000);
		
	//TEST CASE 7: verify there is 1 available block left (0x1000), no blocks the same
		if (mem_dump() == 1 && p[0] != p[1]) {
				x++;
			  printf("G10-TS1: TEST CASE 7 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 7 - FAIL\n");
		}
			
		mem_dealloc(p[1]);
		p[1] = NULL;
		mem_dealloc(p[3]);
		p[3] = NULL;
			
		p[1] = mem_alloc(0x4000 - 2);
		p[2] = mem_alloc(0x1000 - 2);
			
	//TEST CASE 8: verify there are 0 available blocks left, only p[1] is allocated
		if (mem_dump() == 0 && p[1] != NULL && p[2] == NULL) {
				x++;
			  printf("G10-TS1: TEST CASE 8 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 8 - FAIL\n");		
		}
		
		mem_dealloc(p[0]);
		p[0] = NULL;
		
	//TEST CASE 9: verify there is 1 available block left (0x4000)
		if (mem_dump() == 1) {
				x++;
			  printf("G10-TS1: TEST CASE 9 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 9 - FAIL\n");		
		}			

		mem_dealloc(p[1]);
		p[1] = NULL;
		
	//TEST CASE 10: verify there is 1 available block left (0x8000)
		if (mem_dump() == 1) {
				x++;
			  printf("G10-TS1: TEST CASE 10 - PASS\n");
		} else {
				y++;
				printf("G10-TS1: TEST CASE 10 - FAIL\n");		
		}
			
    printf("G10-TS1: %d/10 tests PASSED\r\n", x);
    printf("G10-TS1: %d/10 tests FAILED\r\n", y);
    printf("G10-TS1: END\r\n");
		
//END	
		
    return 0;
}

