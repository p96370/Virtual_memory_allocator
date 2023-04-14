#include "vma.h"

#define MAX_STRING_SIZE 64

block_t *dll_create(const uint64_t address, const uint64_t size)
{
	block_t *list = (block_t *)malloc(sizeof(block_t));
	if (!list) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}
	list->head = NULL;
	list->start_address = address;
	list->size = size;
	return list;
}

void alloc_nth_node(block_t *list, unsigned int n)
{
	if (n < 0)
		return;
	// alloc a new node (miniblock)
	dll_node_t *new_node = malloc(sizeof(*new_node));
	if (!new_node) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}
	new_node->data = (miniblock_t *)malloc(sizeof(miniblock_t));
	if (!new_node->data) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}
	((miniblock_t *)(new_node->data))->rw_buffer = NULL;

	if (!list || !list->head) {
		list->head = new_node;
		list->head->prev = NULL;
		list->head->next = NULL;
		return;
	}

	// add node at the beginning of the block
	if (n == 0) {
		new_node->prev = NULL;
		new_node->next = list->head;
		list->head = new_node;
		return;
	}
	// add node inside of the block or the end of it
	dll_node_t *prev = list->head, *next = list->head->next;
	int i = 0;
	while (i < n - 1 && next) {
		i++;
		prev = next;
		next = next->next;
	}
	new_node->prev = prev;
	new_node->next = next;
	prev->next = new_node;
	// if new node is not inserted at the end
	if (next)
		next->prev = new_node;
}

void cast_miniblock(dll_node_t *node, uint64_t address, uint64_t size)
{
	miniblock_t *info = (miniblock_t *)(node->data);
	info->start_address = address;
	info->size = size;
	info->perm = 6;
	info->rw_buffer = NULL;
}

void alloc_nth_block(list_t *list, unsigned int n, const uint64_t address,
					 const uint64_t size)
{
	if (n < 0)
		return;

	// create a list of miniblocks
	block_t *new_block = dll_create(address, size);

	// add a node in the block as default
	alloc_nth_node(new_block, 0);

	((miniblock_t *)(new_block->head->data))->perm = 6;
	((miniblock_t *)(new_block->head->data))->size = size;
	((miniblock_t *)(new_block->head->data))->start_address = address;
	((miniblock_t *)(new_block->head->data))->rw_buffer = NULL;

	// lista is empty
	if (!list || !list->block_head || list->size == 0) {
		list->block_head = new_block;
		list->block_head->prev = NULL;
		list->block_head->next = NULL;
		list->size++;
		return;
	}

	// alloc block at the beginning of the "big" list
	if (n == 0) {
		new_block->prev = NULL;
		new_block->next = list->block_head;
		list->block_head = new_block;
		// number of blocks in arena increases
		list->size++;
		return;
	}

	// add block inside of list
	block_t *prev = list->block_head, *next = list->block_head->next;
	int i = 0;
	while (i < n - 1 && next) {
		i++;
		prev = next;
		next = next->next;
	}

	// alloc block at the end of the list
	if (n == list->size) {
		new_block->prev = prev;
		new_block->next = NULL;
		prev->next = new_block;
		list->size++;
		return;
	}
	new_block->prev = prev;
	new_block->next = next;
	// corner case uri for adding on the last position
	if (prev)
		prev->next = new_block;
	if (next)
		next->prev = new_block;
	list->size++;
}

void remove_nth_node(block_t *list, int n)
{
	if (!list || !list->head)
		return;

	// coding for the fact that the sole miniblock of the block is to be freed
	if (n == -1) {
		dll_node_t *node = list->head;
		list->size = 0;
		list->head = NULL;
		if (((miniblock_t *)node->data)->rw_buffer)
			free(((miniblock_t *)(node->data))->rw_buffer);
		free(((miniblock_t *)(node->data)));
		free(node);
		return;
	}

	// eliminate the first miniblock
	if (n == 0) {
		dll_node_t *first = list->head;
		list->size -= ((miniblock_t *)(first->data))->size;
		uint64_t adr = ((miniblock_t *)list->head->next->data)->start_address;
		list->start_address = adr;
		list->head = first->next;
		list->head->prev = NULL;
		if (((miniblock_t *)first->data)->rw_buffer)
			free(((miniblock_t *)(first->data))->rw_buffer);
		free(((miniblock_t *)(first->data)));
		free(first);
		return;
	}

	dll_node_t *prev = list->head, *last = list->head->next;
	int i = 0;
	// eliminate miniblock from position i
	while (i < n - 1) {
		prev = last;
		last = last->next;
		++i;
	}
	dll_node_t *current = last;
	list->size -= ((miniblock_t *)last->data)->size;
	// check to see if i is the last position
	if (last->next) {
		current->next->prev = prev;
		prev->next = current->next;
	} else {
		prev->next = NULL;
	}
	if (((miniblock_t *)last->data)->rw_buffer)
		free(((miniblock_t *)(last->data))->rw_buffer);
	free(((miniblock_t *)(last->data)));
	free(last);
}
