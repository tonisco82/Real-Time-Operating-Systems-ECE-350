/**
 * @brief Group 99 Lab1 Memory Test Suite 1 Template File
 */
#include "rtx.h"
#include "uart_polling.h"
#include "printf.h"

int test_mem(void) {

//START
	
		printf("G10-TS3: START\r\n");

		int x = 0;
		int y = 0;
	
		static void *p[1025];
		
		
		for ( int i = 0; i < 1025; i++ ) {
        p[i] = NULL;
    }	
		
/*	
		TEST CASE 1:
			Allocate increasing amounts of memory (leaving gaps in between), then call
			mem_dump() along the way to verify that the correct number of available
			blocks + sizes are displayed (when filling the available blocks). This Test
			Case checks that memory is always allocated in the first available block with
			sufficient space.
*/
		BOOL test1 = 1;
		
		p[0] = mem_alloc(0x400);
		if (mem_dump() != 5) {
				test1 = 0;
		}
		p[2] = mem_alloc(0x800);
		if (mem_dump() != 4) {
				test1 = 0;
		}
		p[4] = mem_alloc(0x2000);
		if (mem_dump() != 3) {
				test1 = 0;
		}
		
		if (test1) {
				x++;
				printf("G10-TS3: TEST CASE 1 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 1 - FAIL\n");
		}

		
/*	
		TEST CASE 2:
			Fill the entire memory block by allocating into the available blocks/gaps, in
			decreasing order: 0x4000 -> 0x1000 -> 0x400. Use mem_dump() along the way to
			verify they are slotted correctly.This Test Case checks that memory is able to
			be allocated into spaces in which they would fit exactly, as well as makes sure
			by allocating the largest block first that we are able to quickly find a large
			enough free block, and that memory is never overwritten. This case fills the
			entire memory space.
*/
		BOOL test2 = 1;
		
		p[5] = mem_alloc(0x4000);
		if (mem_dump() != 2) {
				test2 = 0;
		}
		p[3] = mem_alloc(0x1000);
		if (mem_dump() != 1) {
				test2 = 0;
		}
		p[1] = mem_alloc(0x400);
		if (mem_dump() != 0 || p[0] == p[1]) {
				test2 = 0;
		}

		if (test2) {
				x++;
				printf("G10-TS3: TEST CASE 2 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 2 - FAIL\n");
		}
		
		
/*	
		TEST CASE 3:
			Deallocate all memory blocks in increasing order (size), verifying along the way
			that each freed block is coalesced with its buddy. At each step, there should
			only be one available memory block, which increases in size. Once all memory
			is deallocated, allocate + deallocate a full block (0x8000) to verify that the
			entire freed memory block is reusable. This case empties the entire memory space.
*/
		BOOL test3 = 1;
		
		mem_dealloc(p[0]);
		p[0] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		mem_dealloc(p[1]);
		p[1] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		mem_dealloc(p[2]);
		p[2] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		mem_dealloc(p[3]);
		p[3] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		mem_dealloc(p[4]);
		p[4] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		mem_dealloc(p[5]);
		p[5] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}
		
		p[0] = mem_alloc(0x8000);
		if (mem_dump() != 0 || p[0] == NULL) {
				test3 = 0;
		}
		mem_dealloc(p[0]);
		p[0] = NULL;
		if (mem_dump() != 1) {
				test3 = 0;
		}

		if (test3) {
				x++;
				printf("G10-TS3: TEST CASE 3 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 3 - FAIL\n");
		}
		
		
/*
		TEST CASE 4:
			Repeatedly allocate and deallocate memory. Check to see that no extra memory
			appears or no memory gets lost. The sum of free memory and allocated memory 
			should always be constant.
*/
		int count = 0;
		if (mem_dump() == 1){
			while (count < 8) {
				//printf("%d\n", count);
				p[count] = mem_alloc(0x1000);
				count++;
			}
		}
		if (mem_dump() == 0){
			while (count >= 0) {
				mem_dealloc(p[count]);
				count--;
			}
		}
		count = 0;
		if (mem_dump() == 1){
			while (count < 1024) {
				//printf("%d\n", count);
				p[count] = mem_alloc(0x20);
				count++;
			}
		}
		if (mem_dump() == 0){
			while (count >= 0) {
				mem_dealloc(p[count]);
				count--;
			}
		}
		count = 0;
		if (mem_dump() == 1){
			while (count < 1024) {
				//printf("%d\n", count);
				p[count] = mem_alloc(0x20);
				count++;
			}
		}
		if (mem_dump() == 0){
			// If we made it to this point, after allocating and deallocating memory multiple
			// times, we can still allocate 1024 32 byte memory blocks. Thus did not lose memory.
	
			// There are no more free memory blocks to allocate to, try to allocate one anyways.
			p[count+1] = mem_alloc(0x20);
		}
		if (p[count+1] == NULL){
			// If the pointer is NULL, that means no extra memory appeared.
			x++;
			printf("G10-TS3: TEST CASE 4 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 4 - FAIL\n");
		}			
		while (count >= 0) {
			mem_dealloc(p[count]);
			count--;
		}
		
		
/*
		TEST CASE 5:
			Allocate and deallocate memory with large irregular (non power of 2) value.
			Check to see INTERNAL fragmentation and the correct number of free blocks is
			correctly returned using mem_dump().
*/
		p[0] = mem_alloc(0x4268); // Allocate 17000 bytes
		p[1] = mem_alloc(0xbb8); // Try to allocate 3000 bytes
		p[2] = mem_alloc(0xbb8); // Try to allocate 3000 bytes
		if (mem_dump() == 0 && p[1] == NULL && p[2] == NULL){
			// There should be no free memory blocks.
			// Both pointers should be null as 17000 bytes should take the whole
			// 32K memory block. Thus no more memory can be allocated even though
			// there is plenty of space (internal fragmentation).
			x++;
			printf("G10-TS3: TEST CASE 5 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 5 - FAIL\n");
		}
		mem_dealloc(p[0]);
		mem_dealloc(p[1]);
		mem_dealloc(p[2]);
		
		
/*
		TEST CASE 6:
			Allocate and deallocate memory with small irregular (non power of 2) values.
			Check to see INTERNAL fragmentation and the correct number of free blocks is
			correctly returned using mem_dump().
*/
		p[0] = mem_alloc(0x2328); // Allocate 9000 bytes
		p[1] = mem_alloc(0x5dc); // Allocate 1500 bytes
		p[2] = mem_alloc(0x5dc); // Allocate 1500 bytes
		p[3] = mem_alloc(0x1004); // Allocate 4100 bytes
		p[4] = mem_alloc(0x802); // Allocate 2050 bytes
		p[5] = mem_alloc(0x20); // Try to allocate 32 bytes
		if (mem_dump() == 0 && p[5] == NULL){
			// There should be no free memory blocks to allocate to.
			// p[5] should be NULL even though there is plenty of memory because those
			// blocks are not allocatable due to internal fragmentation.
			x++;
			printf("G10-TS3: TEST CASE 6 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 6 - FAIL\n");
		}
		for (int c = 0; c < 6; c++){
			mem_dealloc(p[c]);
		}
		
		
/*
		TEST CASE 7:
			Allocate and deallocate memory with irregular (non power of 2) values.
			Check to see EXTERNAL fragmentation and the correct number of free blocks is
			correctly returned using mem_dump().
*/
		p[0] = mem_alloc(0x2328); // Allocate 9000 bytes
		p[1] = mem_alloc(0x5dc); // Allocate 1500 bytes
		p[2] = mem_alloc(0x5dc); // Allocate 1500 bytes
		p[3] = mem_alloc(0x1004); // Allocate 4100 bytes
		p[4] = mem_alloc(0x802); // Allocate 2050 bytes
		mem_dealloc(p[0]); // Deallocate 9000 bytes
		mem_dealloc(p[3]); // Deallocate 4100 bytes
		p[5] = mem_alloc(0x4e20); // Try to allocate 24K bytes 
		if (mem_dump() == 2 && p[5] == NULL){
			// There are 2 free memory blocks of size 16K and 8K bytes.
			// Thus there should be enough space to allocate a 23K byte block.
			// But external fragmentation happens since these blocks are not buddys
			// so they do not coaslece, thus p[5] should be NULL.
			x++;
			printf("G10-TS3: TEST CASE 7 - PASS\n");
		} else {
				y++;
				printf("G10-TS3: TEST CASE 7 - FAIL\n");
		}
		mem_dealloc(p[1]);
		mem_dealloc(p[2]);
		mem_dealloc(p[4]);
		mem_dealloc(p[5]);
		
		
		//FINAL RESULTS
    printf("G10-TS3: %d/7 tests PASSED\r\n", x);
    printf("G10-TS3: %d/7 tests FAILED\r\n", y);
    printf("G10-TS3: END\r\n");
	
//END
	
    return 0;
}
