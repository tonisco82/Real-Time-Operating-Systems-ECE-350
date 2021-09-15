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
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"
#include "k_mem_util.h"


IRAM getIRAM(mpool_t mpid) {
    switch (mpid)
    {
    case MPID_IRAM1:
        return IRAM1;
    
    case MPID_IRAM2:
        return IRAM2;
    
    default:
        return IRAM1;
    };

}



/* note list[n] is for blocks with order of n */
mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }

    
    if ( start == RAM1_START) {
        // add your own code
        mpid = MPID_IRAM1;
        IRAM iram = getIRAM(mpid);

        SET_IRAM1_SIZE_LOG2 = log_2(RAM1_END - RAM1_START + 1);
        printf("IRAM1: %d", SET_IRAM1_SIZE_LOG2);

        for (int i = 0; i < h; i++) {
            init_list(&free_lists[iram][i]);
        }

        mark_index_as_unused(0, 0, iram);
    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;
        IRAM iram = getIRAM(mpid);

        SET_IRAM2_SIZE_LOG2 = log_2(RAM2_END - RAM2_START + 1);

        for (int i = 0; i < h; i++) {
            init_list(&free_lists[iram][i]);
        }

        mark_index_as_unused(0, 0, iram);

    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */
    if (mpid != MPID_IRAM1 && mpid != MPID_IRAM2) {
        errno = EINVAL;
        return 0;
    }
    if (size == 0)
    {
        return NULL;
    }

    if (size < MIN_BLK_SIZE) {
        size = MIN_BLK_SIZE;
    }
    
    IRAM iram = getIRAM(mpid);
    size_t height = size_to_height(size, iram);
    node_list* block = NULL;


    while (block == NULL)
    {
        if (!list_empty(&free_lists[iram][height]))
        {
            block = pop_node(&free_lists[iram][height]);
        } else {
            if (height == 0)
            {
                errno = ENOMEM;
                return NULL;
            }
            height--;
        }
    }


    while (size <= height_to_size(height + 1, iram))
    {
        split_block(block, height, iram);
        height++;
        block = pop_node(&free_lists[iram][height]);
    }

    size_t block_index = pointer_to_index(block, height, iram);
    mark_index_as_used(block_index, height, iram);
    
    return block;
}



int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */
    if (!ptr)
    {
        return RTX_OK;
    }
    switch (mpid)
    {
    case MPID_IRAM2:
        if ((uintptr_t)ptr < IRAM2_BASE || (uintptr_t)ptr > IRAM2_BASE + IRAM2_SIZE)
        {
            errno = EFAULT;
            return 0;
        }
        
        break;
    
    case MPID_IRAM1:
        if ((uintptr_t)ptr < IRAM1_BASE || (uintptr_t)ptr > RAM1_END)
        {
            errno = EFAULT;
            return 0;
        }
        
        break;

    default:
        errno = EINVAL;
        return 0;
    }

    IRAM iram = getIRAM(mpid);
    size_t height = pointer_to_height(ptr, iram);
    size_t index = pointer_to_index(ptr, height, iram);
    
    mark_index_as_unused(index, height, iram);

    size_t current_index = index;
    size_t current_height = height;

    while(is_buddy_free(current_index, current_height, iram)) {
        size_t side = get_side(current_index);
        void *pointer = index_to_pointer(current_index, current_height, iram);
        coalesce_buddies(pointer, current_height, side, iram);

        current_index = child_index_to_parent_index(current_index, current_height);
        current_height--;
    }
    
    return RTX_OK; 
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */
    if (mpid != MPID_IRAM1 && mpid != MPID_IRAM2) {
        return 0;
    }

    IRAM iram = getIRAM(mpid);
    size_t count = 0;
    for (int i = h - 1; i >= 0; i--)
    {
        size_t size = height_to_size(i, iram);
        node_list *start = &free_lists[iram][i];
        node_list *current = start;
        while (current->next != start)
        {
            printf("0x%x: 0x%x\r\n", current->next, size);
            current = current->next;
            count++;
        }
    }
    printf("%d free memory block(s) found\r\n", count);
    
    return count;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}


/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
