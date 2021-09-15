/**
 * @brief Group 10 Lab1 Memory Test Suite 2 File
 */
#include "rtx.h"
#include "uart_polling.h"
#include "printf.h"

int test_mem(void) {
	
		static void *p[1024];
		int total_tests = 13;
		int passed_count = 0;
	
		for ( int i = 0; i < 1024; i++ ) {
        p[i] = NULL;
    }
	
		printf("G10-TS2: START\r\n");

		
		// TEST CASE 1: Allocate 1 block of the total 32K bytes of memory.
		p[0] = mem_alloc(0x8000);
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 1 - PASS\n");
			passed_count++;
		}
		mem_dealloc(p[0]);

		
		// TEST CASE 2: Allocate 2 blocks of 16K bytes of memory.
		p[0] = mem_alloc(0x4000);
		p[1] = mem_alloc(0x4000);
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 2 - PASS\n");
			passed_count++;
		}
		mem_dealloc(p[0]);
		mem_dealloc(p[1]);

		
		// TEST CASE 3: Allocate 8 blocks of 4K bytes of memory.
		int count = 0;
		while (count < 8) {
			//printf("%d\n", count);
			p[count] = mem_alloc(0x1000);
			count++;
		}
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 3 - PASS\n");
			passed_count++;
		}
		while (count >= 0) {
			mem_dealloc(p[count]);
			count--;
		}

		
		// TEST CASE 4: Allocate 128 blocks of 256 bytes of memory.
		count = 0;
		while (count < 128) {
			//printf("%d\n", count);
			p[count] = mem_alloc(0x100);
			count++;
		}
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 4 - PASS\n");
			passed_count++;
		}
		while (count >= 0) {
			mem_dealloc(p[count]);
			count--;
		}

		
		// TEST CASE 5: Allocate 256 blocks of 128 bytes of memory.
		count = 0;
		while (count < 256) {
			//printf("%d\n", count);
			p[count] = mem_alloc(0x80);
			count++;
		}
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 5 - PASS\n");
			passed_count++;
		}
		while (count >= 0) {
			mem_dealloc(p[count]);
			count--;
		}


		// TEST CASE 6: Allocate 512 blocks of 64 bytes of memory.
		count = 0;
		while (count < 512) {
			//printf("%d\n", count);
			p[count] = mem_alloc(0x40);
			count++;
		}
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 6 - PASS\n");
			passed_count++;
		}
		while (count >= 0) {
			mem_dealloc(p[count]);
			count--;
		}


		// TEST CASE 7: Allocate 1024 blocks of 32 bytes of memory.
		// This case tests the most extreme case of 1024 blocks of the smallest possible memory size can be allocated.
		// This results in completely allocated memory, all split into 32 byte (smallest possible) portions.
		// This also tests that mem_demp returns correctly after such many allocations.
		count = 0;
		while (count < 1024) {
			//printf("%d\n", count);
			p[count] = mem_alloc(0x20);
			count++;
		}
		if (mem_dump() == 0){
			printf("G10-TS2: TEST CASE 7 - PASS\n");
			passed_count++;
		}

		
		// TEST CASE 8: Deallocate 1 block of 32 bytes at end of list.
		// This case tests that deallocation at the last block does not fail and mem_dump returns properly.
		mem_dealloc(p[1023]);
		if (mem_dump() == 1){
			// There should be 1 free memory block of size 32 bytes.
			printf("G10-TS2: TEST CASE 8 - PASS\n");
			passed_count++;
		}

		
		// TEST CASE 9: Deallocate 1 block of 32 bytes of memory at middle (first block to the right of middle) of list.
		// This case tests that deallocation at a center block does not fail and mem_dump returns properly.
		mem_dealloc(p[512]);
		if (mem_dump() == 2){
			// There should be 2 free memory blocks of size 32 bytes.
			printf("G10-TS2: TEST CASE 9 - PASS\n");
			passed_count++;
		}

		
		// TEST CASE 10: Deallocate block to the left (buddy) of last pointer.
		// This case tests that buddys do coalesce and mem_dump returns properly.
		mem_dealloc(p[1022]);
		if (mem_dump() == 2){
			// There should be 2 free memory blocks. One of 32 bytes and one of 64 bytes as they should coalesce.
			printf("G10-TS2: TEST CASE 10 - PASS\n");
			passed_count++;
		}
		
		
		// TEST CASE 11: Deallocate block to the left (not buddy) of middle-right pointer.
		// This case tests that non-buddys do not coalesce and mem_dump returns properly.
		mem_dealloc(p[511]);
		if (mem_dump() == 3){
			// There should be 3 free memory blocks. Two of 32 bytes and one of 64 bytes.
			printf("G10-TS2: TEST CASE 11 - PASS\n");
			passed_count++;
		}
		
		
		// TEST CASE 12: Reallocate all previously deallocated blocks.
		// This case tests that recently deallocated memory blocks can be reallocated.
		p[511] = mem_alloc(0x20);
		p[512] = mem_alloc(0x20);
		p[1022] = mem_alloc(0x40);
		if (mem_dump() == 0){
			// There should be no free memory blocks.
			printf("G10-TS2: TEST CASE 12 - PASS\n");
			passed_count++;
		}
		
		
		// TEST CASE 13: Deallocate all 1024 allocated 32 byte blocks.
		// This case tests the most extreme deallocation case and verifies mem_demp returns correctly afterwards.
		count = 0;
		while (count < 1023) {
			mem_dealloc(p[count]);
			count++;
		}
		if (mem_dump() == 1){
			// There should be 1 free memory block of size 32K bytes.
			// Because all 1023 32 byte blocks and the 1 64 byte block should all coalesce into 1 32K byte block.
			printf("G10-TS2: TEST CASE 13 - PASS\n");
			passed_count++;
		}
		
		
    printf("G10-TS2: %d/%d tests PASSED\r\n", passed_count, total_tests);
    printf("G10-TS2: %d/%d tests FAILED\r\n", total_tests - passed_count, total_tests);
    printf("G10-TS2: END\r\n");
    return 0;
}
