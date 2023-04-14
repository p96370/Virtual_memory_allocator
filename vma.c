#include "vma.h"

arena_t *alloc_arena(const uint64_t size)
{
	arena_t *new_arena = malloc(sizeof(arena_t));
	new_arena->arena_size = size;
	new_arena->alloc_list = malloc(sizeof(*new_arena->alloc_list));

	if (!new_arena->alloc_list) {
		fprintf(stderr, "Malloc failed\n");
		return NULL;
	}
	new_arena->alloc_list->block_head = NULL;
	// number of blocks
	new_arena->alloc_list->size = 0;
	return new_arena;
}

void dealloc_arena(arena_t *arena)
{
	block_t *curr_block = arena->alloc_list->block_head;
	while (curr_block) {
		while (curr_block->head) {
			dll_node_t *node = curr_block->head;
			curr_block->head = curr_block->head->next;
			if (((miniblock_t *)node->data)->rw_buffer)
				free(((miniblock_t *)(node->data))->rw_buffer);
			free(((miniblock_t *)(node->data)));
			free(node);
		}
		block_t *block = curr_block;
		curr_block = curr_block->next;
		free(block);
	}
	free(arena->alloc_list);
}

void alloc_middle_node(block_t *current_block, int i)
{
	// insert miniblock in the middle  of the block
	size_t copy_size = current_block->size;
	dll_node_t *sec_node = current_block->next->head;
	dll_node_t *node = current_block->head;
	while (node->next)
		node = node->next;
	block_t *aux_block = current_block->next;
	while (sec_node) {
		i++;
		alloc_nth_node(current_block, i);
		node = node->next;
		uint64_t new_adr = ((miniblock_t *)sec_node->data)->start_address;
		size_t new_size = ((miniblock_t *)sec_node->data)->size;
		cast_miniblock(node, new_adr, new_size);
		dll_node_t *aux_node = sec_node;
		sec_node = sec_node->next;
		free(((miniblock_t *)(aux_node->data))->rw_buffer);
		free(((miniblock_t *)(aux_node->data)));
		free(aux_node);
	}
	current_block->next = aux_block->next;
	if (aux_block->next)
		aux_block->next->prev = current_block;
	free(aux_block);
	current_block->size = copy_size;
}

void alloc_new_block_head(arena_t *arena, uint64_t address, uint64_t size)
{
	alloc_nth_node(arena->alloc_list->block_head, 0);
	// size of the first block increases
	arena->alloc_list->block_head->size += size;
	cast_miniblock(arena->alloc_list->block_head->head, address, size);
	arena->alloc_list->block_head->start_address = address;
}

int alloc_messages(uint64_t my_size, uint64_t address, uint64_t size)
{
	if (address >= my_size) {
		printf("The allocated address is outside the size of arena\n");
		return 0;
	}
	if (address + size > my_size) {
		printf("The end address is past the size of the arena\n");
		return 0;
	}
	return 1;
}

int alloc_beginning(arena_t *arena, uint64_t address, uint64_t size)
{
	block_t *current_block = arena->alloc_list->block_head;
	size_t curr_block_size = arena->alloc_list->block_head->size;
	uint64_t first_address = arena->alloc_list->block_head->start_address;

	if (address + size < first_address) {
		alloc_nth_block(arena->alloc_list, 0, address, size);
		return 1;
	} else if (address + size == first_address) {
		//insert miniblock at the beginning of the first block
		if (current_block->start_address + curr_block_size == first_address) {
			alloc_new_block_head(arena, address, size);
			return 1;
			// if the new zone overlaps a previously allocated zone
		} else if (address < first_address && address + size > first_address) {
			printf("This zone was already allocated.\n");
			return 1;
		}
	}
	return 0;
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	if (alloc_messages(arena->arena_size, address, size) == 0)
		return;
	// list is empty, it doesn't have any blocks
	if (!arena->alloc_list->block_head || arena->alloc_list->size == 0) {
		alloc_nth_block(arena->alloc_list, 0, address, size);
		arena->alloc_list->size = 1;
		return;
	}
	block_t *current_block = arena->alloc_list->block_head;
	size_t curr_block_size = arena->alloc_list->block_head->size;
	uint64_t first_address = arena->alloc_list->block_head->start_address;

	// if I add a block to the beginning of the list of blocks
	if (alloc_beginning(arena, address, size))
		return;

	int i = 0;
	// when the list has at least 1 element
	while (current_block->start_address + curr_block_size < address) {
		current_block = current_block->next;
		i++;
		if (current_block)
			curr_block_size = current_block->size;
		else
			break;
	}
	// add a new block to the end of the list
	if (!current_block) {
		alloc_nth_block(arena->alloc_list, arena->alloc_list->size,
						address, size);
		return;
	}
	if (current_block->next)
		if (address + size > current_block->next->start_address) {
			printf("This zone was already allocated.\n");
			return;
		}
	if (address + size == current_block->start_address) {
		// insert miniblock at the beginning of the block
		alloc_nth_node(current_block, 0);
		cast_miniblock(current_block->head, address, size);
		current_block->start_address = address;
		current_block->size += size;
		return;
	}
	if (current_block->start_address + curr_block_size == address) {
		// insert a miniblock at the end of the block
		// and maybe later I attach the next block
		dll_node_t *node = current_block->head;
		int i = 1; // position to add miniblock
		while (node->next) {
			node = node->next;
			i++;
		}
		alloc_nth_node(current_block, i);
		current_block->size += size;
		cast_miniblock(node->next, address, size);
		if (current_block->next) {
			if (address + size == current_block->next->start_address) {
				// insert miniblock in the middle  of the block
				arena->alloc_list->size--; // scad nr de block uri
				current_block->size += current_block->next->size;
				alloc_middle_node(current_block, i);
			}
		}
		return;
	}
	// insert new block inside list
	if (address + size < current_block->start_address)
		alloc_nth_block(arena->alloc_list, i, address, size);
	if (address + size > current_block->start_address)
		printf("This zone was already allocated.\n");
}

void copy_miniblock(dll_node_t *a, dll_node_t *b)
{
	((miniblock_t *)a->data)->size = ((miniblock_t *)b->data)->size;
	((miniblock_t *)a->data)->start_address =
	((miniblock_t *)b->data)->start_address;
	((miniblock_t *)a->data)->rw_buffer = ((miniblock_t *)b->data)->rw_buffer;
	((miniblock_t *)a->data)->perm = ((miniblock_t *)b->data)->perm;
}

void free_aux(dll_node_t **aux)
{
	if (((miniblock_t *)(*aux)->data)->rw_buffer)
		free(((miniblock_t *)(*aux)->data)->rw_buffer);
	free((*aux)->data);
	free(*aux);
}

void init_block(block_t *block, dll_node_t *node, size_t size)
{
	block->start_address = ((miniblock_t *)node->data)->start_address;
	block->size = size;
	((miniblock_t *)(block->head->data))->size =
	((miniblock_t *)(node->data))->size;
	((miniblock_t *)block->head->data)->start_address =
	((miniblock_t *)node->data)->start_address;
}

void free_node_and_block(block_t *prev, arena_t *arena, block_t *block,
						 block_t **aux)
{
	// if the first block is to be freed
	if (!prev) {
		arena->alloc_list->block_head = block->next;
		arena->alloc_list->block_head->prev = NULL;
		free((*aux)->head);
		free(*aux);
		arena->alloc_list->size--;
		return;
	}
	// if the last block is to be freed
	if (!block->next) {
		prev->next = NULL;
		free((*aux)->head);
		free(*aux);
		arena->alloc_list->size--;
		return;
	}
	// the block to be freed in the middle of the list
	// list has at least 3 blocks
	arena->alloc_list->size--;
	prev->next = (*aux)->next;
	(*aux)->next->prev = prev;
	free((*aux)->head);
	free(*aux);
}

void free_middle_node(arena_t *arena, block_t *block, dll_node_t *node,
					  size_t sum, int poz_block, int poz)
{
	uint64_t new_block_adr, new_block_size;
	size_t new_size_left, new_size_right;
	dll_node_t *aux2 = node->next;
	new_block_adr = ((miniblock_t *)(aux2->data))->start_address;
	new_block_size = block->size - sum;
	new_size_left = sum - ((miniblock_t *)(node->data))->size;
	new_size_right = block->size - sum;
	// node->next will be initialized in function
	dll_node_t *sec_node = node->next->next;
	alloc_nth_block(arena->alloc_list, poz_block + 1, new_block_adr,
					new_block_size);
	block_t *copy = block;
	// becomes the newly allocated block
	block = block->next;
	init_block(block, node->next, new_size_right);
	dll_node_t *new_alloc_node = block->head;
	int new_pos = 1;
	while (sec_node) {
		alloc_nth_node(block, new_pos);
		new_alloc_node = new_alloc_node->next;
		copy_miniblock(new_alloc_node, sec_node);
		new_pos++;
		dll_node_t *aux = sec_node;
		sec_node = sec_node->next;
		free_aux(&aux);
	}
	// last node before the one that must be freed
	dll_node_t *prev_node = node->prev;
	remove_nth_node(copy, poz);
	free_aux(&aux2);
	copy->size = new_size_left;
	prev_node->next = NULL;
}

// frees a miniblock
void free_block(arena_t *arena, const uint64_t address)
{
	block_t *curr_block = arena->alloc_list->block_head;
	if (arena->alloc_list->size == 1) {
		// eliminate the only block (which has only one miniblock) of the list
		if (curr_block->start_address == address && !curr_block->head->next) {
			remove_nth_node(curr_block, -1);
			free(curr_block);
			arena->alloc_list->block_head = NULL;
			arena->alloc_list->size = 0;
			return;
		}
	}
	int poz_block = 0;
	block_t *prev = NULL;
	while (curr_block) {
		dll_node_t *node = curr_block->head;
		size_t size_minib = 0, sum_size_minib = 0;
		int poz = 0;
		while (node) {
			sum_size_minib += ((miniblock_t *)(node->data))->size;
			// found the wanted miniblock
			if (((miniblock_t *)node->data)->start_address ==  address) {
				size_minib = ((miniblock_t *)(node->data))->size;
				// there is only one miniblock in block so free the block too
				if (size_minib == curr_block->size) {
					block_t *aux = curr_block;
					remove_nth_node(curr_block, -1);
					free_node_and_block(prev, arena, curr_block, &aux);
					return;
				}
				// if the miniblock is at the end of the list block_t
				if (!node->next) {
					remove_nth_node(curr_block, poz);
					return;
				}
				// if the miniblock is at the beginning of the list block_t
				if (!node->prev) {
					remove_nth_node(curr_block, 0);
					return;
				}
				// free in the middle
				// split into 2 blocks the current block
				if (!(!node->prev || !node->next)) {
					free_middle_node(arena, curr_block, node, sum_size_minib,
									 poz_block, poz);
					return;
				}
				break;
			}
			poz++;
			node = node->next;
		}
		prev = curr_block;
		curr_block = curr_block->next;
		poz_block++;
	}
	printf("Invalid address for free.\n");
}

uint64_t min(uint64_t a, uint64_t b)
{
	if (a > b)
		return b;
	return a;
}

int is_valid_read(dll_node_t *node)
{
	int perm = ((miniblock_t *)(node->data))->perm;
	if (perm == 4 || perm == 7 || perm == 6 || perm == 5)
		return 1;
	return 0;
}

void read(arena_t *arena, uint64_t address, uint64_t size)
{
	if (!arena->alloc_list) {
		printf("Invalid address for read.\n");
		return;
	}
	if (!arena->alloc_list->block_head) {
		printf("Invalid address for read.\n");
		return;
	}

	block_t *curr_block = arena->alloc_list->block_head;
	while (curr_block) {
		size_t sum_partial = 0;
		dll_node_t *node = curr_block->head;
		while (node) {
			uint64_t left_lim = ((miniblock_t *)node->data)->start_address;
			uint64_t right_lim = left_lim + ((miniblock_t *)node->data)->size;
			// found the wanted miniblock
			if (address >= left_lim && address < right_lim) {
				// read as much data as the size allows
				if (size > curr_block->size - sum_partial) {
					printf("Warning: size was bigger than the block size.");
					size_t val = curr_block->size - sum_partial;
					printf(" Reading %ld characters.\n", val);
				}
				dll_node_t *aux_node = node;
				uint64_t copy_size = size;
				// while there are miniblocks left and data to read
				while (aux_node && copy_size > 0) {
					size_t my_size = ((miniblock_t *)aux_node->data)->size;
					uint64_t length = min(my_size, copy_size);
					copy_size -= length;
					if (is_valid_read(aux_node) == 0) {
						printf("Invalid permissions for read.\n");
						return;
					}
					aux_node = aux_node->next;
				}

				while (node && size > 0) {
					size_t my_size = ((miniblock_t *)node->data)->size;
					uint64_t length = min(my_size, size);
					char *s = (char *)(((miniblock_t *)node->data)->rw_buffer);
					while (address > left_lim) {
						s++;
						address--;
					}
					for (int i = 0; i < length; i++)
						printf("%c", s[i]);

					size -= length;
					node = node->next;
				}
				printf("\n");
				return;
			}
			sum_partial += ((miniblock_t *)node->data)->size;
			node = node->next;
		}
		curr_block = curr_block->next;
	}
	printf("Invalid address for read.\n");
}

int is_valid_write(dll_node_t *node)
{
	int perm = ((miniblock_t *)node->data)->perm;
	if (perm == 3 || perm == 7 || perm == 6 || perm == 2)
		return 1;

	return 0;
}

void write(arena_t *arena, const uint64_t address, uint64_t size, int8_t *data)
{
	if (!arena->alloc_list) {
		printf("Invalid address for write.\n");
		return;
	}
	if (!arena->alloc_list->block_head) {
		printf("Invalid address for write.\n");
		return;
	}
	block_t *curr_block = arena->alloc_list->block_head;
	while (curr_block) {
		size_t sum_partial = 0;
		dll_node_t *node = curr_block->head;
		while (node) {
			uint64_t left_lim = ((miniblock_t *)node->data)->start_address;
			uint64_t right_lim = left_lim + ((miniblock_t *)node->data)->size;
			// found the requested address
			if (address >= left_lim && address < right_lim) {
				dll_node_t *aux_node = node;
				uint64_t copy_size = size;

				while (aux_node && copy_size > 0) {
					size_t my_size = ((miniblock_t *)aux_node->data)->size;
					uint64_t length = min(my_size, copy_size);
					copy_size -= length;
					if (is_valid_write(aux_node) == 0) {
						printf("Invalid permissions for write.\n");
						return;
					}
					aux_node = aux_node->next;
				}
				while (left_lim < address) {
					left_lim++;
					sum_partial++;
				}
				// checks size of the requested data
				// do not write more than allowed
				if (size > curr_block->size - sum_partial) {
					printf("Warning: size was bigger than the block size.");
					size_t val = curr_block->size - sum_partial;
					printf(" Writing %ld characters.\n", val);
				}
				// while there are miniblocks left and data to write
				while (node && size > 0) {
					size_t my_size = ((miniblock_t *)node->data)->size;
					((miniblock_t *)node->data)->rw_buffer = malloc(my_size);
					if (!((miniblock_t *)node->data)->rw_buffer) {
						fprintf(stderr, "Malloc failed\n");
						return NULL;
					}
					uint64_t len = min(my_size, size);
					strncpy(((miniblock_t *)node->data)->rw_buffer, data, len);
					char *s = (char *)(((miniblock_t *)node->data)->rw_buffer);
					data = data + len;
					size -= len;
					node = node->next;
				}
				return;
			}
			sum_partial += ((miniblock_t *)node->data)->size;
			node = node->next;
		}
		curr_block = curr_block->next;
	}
	printf("Invalid address for write.\n");
}

void print_permissions(int8_t perm)
{
	if (perm == 0)
		printf("---");
	else if (perm == 1)
		printf("--X");
	else if (perm == 2)
		printf("-W-");
	else if (perm == 4)
		printf("R--");
	else if (perm == 6)
		printf("RW-");
	else if (perm == 5)
		printf("R-X");
	else if (perm == 3)
		printf("-WX");
	else if (perm == 7)
		printf("RWX");
	printf("\n");
}

void pmap(const arena_t *arena)
{
	printf("Total memory: 0x%lX bytes\n", arena->arena_size);
	block_t *curr_block = arena->alloc_list->block_head;
	uint64_t occ_mem = 0;
	size_t nr_miniblocks = 0;
	while (curr_block) {
		occ_mem += curr_block->size;
		dll_node_t *miniblock = curr_block->head;
		while (miniblock) {
			miniblock = miniblock->next;
			nr_miniblocks++;
		}
		curr_block = curr_block->next;
	}
	printf("Free memory: 0x%lX bytes\n", arena->arena_size - occ_mem);
	printf("Number of allocated blocks: %u\n", arena->alloc_list->size);
	printf("Number of allocated miniblocks: %lu\n", nr_miniblocks);

	curr_block = arena->alloc_list->block_head;
	for (int i = 0; i < arena->alloc_list->size; i++) {
		printf("\nBlock %d begin\n", i + 1);
		size_t dim_total = curr_block->start_address + curr_block->size;
		printf("Zone: 0x%lX - 0x%lX", curr_block->start_address, dim_total);
		int poz = 1;
		dll_node_t *node = curr_block->head;
		uint64_t address = ((miniblock_t *)node->data)->start_address;
		size_t size_min = ((miniblock_t *)node->data)->size;
		while (node) {
			if (poz == 1)
				printf("\n");
			printf("Miniblock %d:\t\t0x%lX\t\t-\t\t", poz, address);
			printf("0x%lX\t\t| ", address + size_min);
			//RW-\n
			print_permissions(((miniblock_t *)node->data)->perm);
			poz++;
			address = address + size_min;
			node = node->next;
			if (node)
				size_min = ((miniblock_t *)node->data)->size;
		}
		printf("Block %d end\n", i + 1);
		curr_block = curr_block->next;
	}
}

int is_alpha(char c)
{
	if (c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	block_t *curr_block = arena->alloc_list->block_head;
	int found = 0;
	dll_node_t *node;
	while (curr_block && !found) {
		dll_node_t *curr_node = curr_block->head;
		while (curr_node && !found) {
			if (((miniblock_t *)curr_node->data)->start_address == address) {
				found = 1;
				node = curr_node;
			}
			curr_node = curr_node->next;
		}
		curr_block = curr_block->next;
	}
	if (found == 0) {
		printf("Invalid address for mprotect.\n");
		return;
	}
	int first_time = 0;
	permission = permission + 5;
	while (permission[0] != '\0') {
		if (permission[0] == 'N') {
			((miniblock_t *)node->data)->perm = 0;
			permission = permission + 4;
		} else if (permission[0] == 'E') {
			if (!first_time) {
				((miniblock_t *)node->data)->perm = 1;
				first_time = 1;
			} else {
				((miniblock_t *)node->data)->perm += 1;
			}
			permission = permission + 4;
		} else if (permission[0] == 'R') {
			if (!first_time) {
				((miniblock_t *)node->data)->perm = 4;
				first_time = 1;
			} else {
				((miniblock_t *)node->data)->perm += 4;
			}
			permission = permission + 4;
		} else { // W
			if (!first_time) {
				((miniblock_t *)node->data)->perm = 2;
				first_time = 1;
			} else {
				((miniblock_t *)node->data)->perm += 2;
			}
			permission = permission + 5;
		}
		while (!is_alpha(permission[0]) && permission[0] != '\0')
			permission++;
		if (permission[0] != '\0')
			permission = permission + 5;
	}
}
