#include "k_mem_util.h"

// the doubly linked list that contains the free nodes at every height
node_list free_lists[NUMBER_OF_RAMS][h];

// the binary tree that holds the used blocks of memory
uint8_t  used_blocks[NUMBER_OF_RAMS][(1 << h) / 8];

size_t SET_IRAM2_SIZE_LOG2 = IRAM2_SIZE_LOG2;
size_t SET_IRAM1_SIZE_LOG2 = IRAM1_SIZE_LOG2;
size_t SET_IRAM1_BASE = IRAM1_BASE;


uint8_t get_used_block(size_t index, IRAM iram) {
    // floor(index / 8) to get which entry in the array
    size_t array_index = index / 8;
    size_t bit_index = index % 8;

    uint8_t bits = used_blocks[iram][array_index];
    return (bits >> bit_index) & 1;
}

void set_used_block(size_t index, size_t value, IRAM iram) {
    // floor(index / 8) to get which entry in the array
    size_t array_index = index / 8;
    size_t bit_index = index % 8;

    if (value) {
        used_blocks[iram][array_index]  |= 1UL << bit_index;
    } else {
        used_blocks[iram][array_index]  &= ~(1UL << bit_index);
    }
}


void push_node(node_list *list, node_list *node) {
    // Head of list contains the pointer to the tail
    node_list *old_tail = list->prev;
    node_list *new_tail = node;

    // Old tail now points to the new tail
    old_tail->next = new_tail;
    // the new tail should be after the old tail
    new_tail->prev = old_tail;
    // the next of the new tail should be the head 
    new_tail->next = list;

    // the previous of the head should be the new tail
    list->prev = new_tail;
}

// Only one copy of a pointer should be in the free list at time
void remove_node(node_list *node) {
    node_list *prev = node->prev;
    node_list *next = node->next;

    prev->next = next;
    next->prev = prev;
}

node_list* pop_node(node_list *list) {
    node_list *front = list->next;

    // This would be the pointer that stores the head/tail
    if (front == list) {
        return NULL;
    }

    remove_node(front);
    return front;
}

size_t list_empty(node_list *list) {
    // check to see if the next and previous of the list
    // both point to the head, which is not a real node
    if (list->next == list && list->prev == list) {
        return 1;
    }

    return 0;
}

// Initialize the list such that we can check if the list is empty
// also creates the circular doubly linked list
void init_list(node_list *list) {
    list->next = list;
    list->prev = list;
}

size_t index_to_relative_index(size_t index, size_t height) {
    size_t relative_index = index - (1 << height) + 1;
    return relative_index;
}

size_t relative_index_to_index(size_t relative_index, size_t height) {
    return (1 << height) + relative_index - 1;
}

void *index_to_pointer(size_t index, size_t height, IRAM iram) {
    size_t relative_index = index_to_relative_index(index, height);
    size_t block_size = height_to_size(height, iram);
    size_t IRAM_BASE = iram == IRAM1 ? SET_IRAM1_BASE : IRAM2_BASE;
    return (void *)((block_size * relative_index) + IRAM_BASE);
}

size_t pointer_to_index(void *block, size_t height, IRAM iram) {
    size_t block_size = height_to_size(height, iram);
    
    // cast this to a type you can do arithmetic on
    // see: https://stackoverflow.com/questions/24472724/expression-must-be-a-pointer-to-a-complete-object-type-using-simple-pointer-arit
    uintptr_t block_c = (uintptr_t)block;

    size_t IRAM_BASE = iram == IRAM1 ? SET_IRAM1_BASE : IRAM2_BASE;

    size_t relative_index = ((block_c - IRAM_BASE) / block_size);
    return relative_index_to_index(relative_index, height);
}

size_t size_to_height(size_t size, IRAM iram) {
    
    // If the size is less than the MIN_BLK_SIZE use the min height
    if (size < MIN_BLK_SIZE)
    {
        return h - 1;
    }

    // Get the floor(log_2(size)) and if its not an exact fit, increment the height by 1
    size_t block_size_log_2 = log_2(size);

    if ((1 << block_size_log_2) < size)
    {
        block_size_log_2++;
    }

    size_t IRAM_SIZE_LOG2 = iram == IRAM1 ? SET_IRAM1_SIZE_LOG2 : SET_IRAM2_SIZE_LOG2;
    size_t height = IRAM_SIZE_LOG2 - block_size_log_2;
    return height;
}

size_t height_to_size(size_t height, IRAM iram){
    size_t IRAM_SIZE_LOG2 = iram == IRAM1 ? SET_IRAM1_SIZE_LOG2 : SET_IRAM2_SIZE_LOG2;
    return 1 << (IRAM_SIZE_LOG2 - height);
}

size_t log_2(size_t n) {
    if (n == 0){
        return 0;
    }
    // _builtin_clz is a gcc builtin that compute the number of leading 0's
    // the log_2 of a function is the largest index of a 1 bit.
    // see: https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
    size_t leading_zeros = __builtin_clz(n);
    size_t size = sizeof(size_t) * 8;
    return size - leading_zeros - 1;
}


size_t parent_index_to_child_index(size_t parent_index, size_t height, size_t side) {
    size_t relative_index = index_to_relative_index(parent_index, height);
    size_t child_offset = (1 << (height + 1));
    size_t child_index = child_offset + (2 * relative_index) - 1 + side;
    return child_index;
}

size_t child_index_to_parent_index(size_t child_index, size_t height) {
    size_t parent_offset = (1 << (height - 1));
    size_t relative_index = index_to_relative_index(child_index, height);

    size_t parent_index = parent_offset + (relative_index >> 1) - 1;
    return parent_index;
}

void mark_index_as_used(size_t index, size_t height, IRAM iram) {
    set_used_block(index, 1, iram);
    void *pointer = index_to_pointer(index, height, iram);
    remove_node(pointer);
}

void mark_index_as_unused(size_t index, size_t height, IRAM iram) {
    set_used_block(index, 0, iram);
    void *pointer = index_to_pointer(index, height, iram);
    push_node(&free_lists[iram][height], pointer);
}

void split_block(void * block, size_t height, IRAM iram) {
    size_t parent_index = pointer_to_index(block, height, iram);

    // If the block is used we cannot split it
    if (get_used_block(parent_index, iram) == 1) {
        printf("ERROR: tried to split used block");
        return;
    }

    // because we are splitting the parent it is used
    mark_index_as_used(parent_index, height, iram);

    // get the index of the newly created children
    size_t left_child_index = parent_index_to_child_index(parent_index, height, left);
    size_t right_child_index = parent_index_to_child_index(parent_index, height, right);

    // the children are new, so they are unused
    mark_index_as_unused(left_child_index, height + 1, iram);
    mark_index_as_unused(right_child_index, height + 1, iram);
}


void coalesce_buddies(void *block, size_t height, size_t side, IRAM iram) {
    // note: side is 0 for left, 1 for right. This allows us to easily get
    //       the right node
    size_t left_child_index = pointer_to_index(block, height, iram) - side;
    size_t right_child_index = left_child_index + 1;


    void *left_child_pointer = index_to_pointer(left_child_index, height, iram);
    void *right_child_pointer = index_to_pointer(right_child_index, height, iram);

    // Remove the children from the free list
    // note: they should already be 0 in the tree because they are free
    remove_node(left_child_pointer);
    remove_node(right_child_pointer);


    // add the parent to the free list
    size_t parent_index = child_index_to_parent_index(left_child_index, height);
    mark_index_as_unused(parent_index, height - 1, iram);
}

size_t pointer_to_height(void *block, IRAM iram) {
    size_t height = 0;
    size_t current_index = 0;

    while (height < h) {
        current_index = pointer_to_index(block, height, iram);
        size_t is_used = get_used_block(current_index, iram);
        if (is_used != 1) {
            break;
        }
        height++;
    }
    height--;
    return height;
}


size_t get_side(size_t index) {
    size_t side = (index % 2) == 0;
    return side;
}

size_t is_buddy_free(size_t index, size_t height, IRAM iram) {
    if (!height) {
        return 0;
    }

    size_t buddy_index = index + ((get_side(index) == right) ? -1 : 1); 
    return !get_used_block(buddy_index, iram);
}
