#ifndef __VMA_H__
#define __VMA_H__

#pragma once
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct miniblock_t {
	uint64_t start_address;
	size_t size; // size of the miniblock
	uint8_t perm;
	void *rw_buffer;
} miniblock_t;

// a node of the second doubly linked list
typedef struct dll_node_t {
	void *data; // cast of tipe miniblock_t
	struct dll_node_t *prev, *next;
} dll_node_t;

typedef struct block_t {
	uint64_t start_address;
	size_t size; // sum of all sizes of miniblocks in a block
	dll_node_t *head; // head-ul of the list of miniblocks
	struct block_t *next, *prev;
} block_t;

typedef struct list_t { // doubly linked list of blocks
	block_t *block_head; // the head of each block
	unsigned int size; // number of blocks
} list_t;

typedef struct arena_t {
	uint64_t arena_size;
	list_t *alloc_list; // head of list of blocks
} arena_t;

block_t *dll_create(const uint64_t address, const uint64_t size);
void alloc_nth_node(block_t *list, unsigned int n);
void cast_miniblock(dll_node_t *node, const uint64_t address,
					const uint64_t size);
void alloc_nth_block(list_t *list, unsigned int n, const uint64_t address,
					 const uint64_t size);

arena_t *alloc_arena(const uint64_t size);
void dealloc_arena(arena_t *arena);
void alloc_middle_node(block_t *current_block, int i);
void alloc_new_block_head(arena_t *arena, uint64_t address, uint64_t size);
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
void copy_miniblock(dll_node_t *a, dll_node_t *b);
void free_aux(dll_node_t **aux);
void init_block(block_t *block, dll_node_t *node, size_t size);
void free_block(arena_t *arena, const uint64_t address);
void remove_nth_node(block_t *list, int n);
int is_valid(dll_node_t *node, uint64_t address, char c);
void read(arena_t *arena, uint64_t address, uint64_t size);
uint64_t min(uint64_t a, uint64_t b);
void write(arena_t *arena, const uint64_t address,  uint64_t size,
		   int8_t *data);
void pmap(const arena_t *arena);
void print_permissions(int8_t perm);
int is_alpha(char c);
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);

#endif
