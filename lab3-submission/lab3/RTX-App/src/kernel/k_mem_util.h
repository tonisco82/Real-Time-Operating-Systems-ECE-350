#ifndef K_MEM_UTIL_H_
#define K_MEM_UTIL_H_
#include "k_inc.h"
#include "k_mem.h"
#include "uart_polling.h"
#include "printf.h"

#define h (IRAM2_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 + 1)
#define left 0
#define right 1

// Circular linked list struct
typedef struct node_list
{
	struct node_list *next;
	struct node_list *prev;
} node_list;

typedef enum IRAM
{
	IRAM1,
	IRAM2,
	NUMBER_OF_RAMS
} IRAM;

// the doubly linked list that contains the free nodes at every height
extern node_list free_lists[NUMBER_OF_RAMS][h];

// the binary tree that holds the used blocks of memory
extern uint8_t used_blocks[NUMBER_OF_RAMS][(1 << h) / 8];

extern size_t SET_IRAM2_SIZE_LOG2;
extern size_t SET_IRAM1_SIZE_LOG2;
extern size_t SET_IRAM1_BASE;

// Binary Tree Functions
// ---------------------------------------------------------------------------------------

// Gets the value for an index in the binary tree
uint8_t get_used_block(size_t index, IRAM iram);

// Sets the value at an index for the binary tree
void set_used_block(size_t index, size_t value, IRAM iram);
// ---------------------------------------------------------------------------------------

// Linked List functions
// ---------------------------------------------------------------------------------------

// Pushes a node to the back of the linked list
void push_node(node_list *list, node_list *node);

// Remove a node from its linked list
// note: it does not need to know which list, because it is circular
void remove_node(node_list *node);

// Removes the node from the front of the list
// note: the static node_list that is allocated is not a valid node
//       and it is not at the front of the list
node_list *pop_node(node_list *list);

// Checks if list is empty
size_t list_empty(node_list *list);

// Initializes the list to point to itself as its circular
void init_list(node_list *list);
// ---------------------------------------------------------------------------------------

// Utilities
// ---------------------------------------------------------------------------------------

// Converts an index in the binary tree to the pointer in memory for that block
void *index_to_pointer(size_t index, size_t height, IRAM iram);

// Converts a pointer to a memory block to an index in the binary tree
size_t pointer_to_index(void *block, size_t height, IRAM iram);

// Calculates at what height a given block size is found using the binary tree
size_t size_to_height(size_t size, IRAM iram);

// Calculates what size a memory block at a given height should be
size_t height_to_size(size_t height, IRAM iram);

// Calculates the floor(log_2(n)) of the input number
size_t log_2(size_t n);

// Converts index in binary tree to relative index in level of tree
// relative index, is the index starting from 0 at a given level
size_t index_to_relative_index(size_t index, size_t height);

// Converts relative index in level of tree to index in binary tree
// the absolute index, is the BFS traversal index
size_t relative_index_to_index(size_t relative_index, size_t height);

// Gets the left or right child index from a parent's index
// see: https://www.notion.so/moyez/Implementation-Documentation-9f8170addf384a018d8bbe587e4ebbb3
size_t parent_index_to_child_index(size_t parent_index, size_t height, size_t side);

// Gets the parent index given the child index
// see: https://www.notion.so/moyez/Implementation-Documentation-9f8170addf384a018d8bbe587e4ebbb3
size_t child_index_to_parent_index(size_t child_index, size_t height);

// Return which side the index is on
size_t get_side(size_t index);
// ---------------------------------------------------------------------------------------

// Core Buddy System Functions
// ---------------------------------------------------------------------------------------

// Marks a memory block as used in the binary tree
void mark_index_as_used(size_t index, size_t height, IRAM iram);

// Marks a memory block as unused in the binary tree
void mark_index_as_unused(size_t index, size_t height, IRAM iram);

// Finds the height a pointer to a memory block belongs to by finding the smallest block
// that is used for that pointer in the binary tree
size_t pointer_to_height(void *block, IRAM iram);

// Splits a parent block, marks itself as used and its children as free
void split_block(void *block, size_t height, IRAM iram);

// Combines to buddies by removing them from the free list and setting the
// parent to be free
void coalesce_buddies(void *block, size_t height, size_t side, IRAM iram);

// checks to see if the buddy of the index is free
size_t is_buddy_free(size_t index, size_t height, IRAM iram);
// ---------------------------------------------------------------------------------------

#endif
