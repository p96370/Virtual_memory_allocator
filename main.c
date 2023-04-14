#include "vma.h"

int main(void)
{
	char *command = malloc(50);
	arena_t *arena = NULL;
	while (1) {
		scanf("%s", command);
		int ok = 0;
		if (strcmp(command, "ALLOC_ARENA") == 0) {
			long size;
			scanf("%ld", &size);
			arena = alloc_arena(size);
			ok = 1;
		}
		if (strcmp(command, "ALLOC_BLOCK") == 0) {
			long start_address;
			size_t size;
			scanf("%ld%lu", &start_address, &size);
			alloc_block(arena, start_address, size);
			ok = 1;
		}
		if (strcmp(command, "FREE_BLOCK") == 0) {
			long adr;
			scanf("%ld", &adr);
			free_block(arena, adr);
			ok = 1;
		}
		if (strcmp(command, "READ") == 0) {
			long adr, size;
			scanf("%ld%ld", &adr, &size);
			read(arena, adr, size);
			ok = 1;
		}
		if (strcmp(command, "WRITE") == 0) {
			ok = 1;
			char *data = malloc(500 * sizeof(char));
			if (!data) {
				fprintf(stderr, "Malloc failed\n");
				return 0;
			}
			unsigned long address, size;
			char c;
			scanf("%ld%ld%c", &address, &size, &c);
			fgets(data, 500, stdin);
			if (c == '\n') {
				char aux[500];
				strcpy(aux, data);
				strcpy(data, "\n");
				strcat(data, aux);
			}
			while (strlen(data) < size) {
				char *sec_data = malloc(200);
				fgets(sec_data, 200, stdin);
				strcat(data, sec_data);
				free(sec_data);
			}
			write(arena, address, size, (int8_t *)data);
			free(data);
		}
		if (strcmp(command, "PMAP") == 0) {
			pmap(arena);
			ok = 1;
		}
		if (strcmp(command, "DEALLOC_ARENA") == 0) {
			free(command);
			dealloc_arena(arena);
			free(arena);
			exit(0);
		}
		if (strcmp(command, "MPROTECT") == 0) {
			uint64_t address;
			int8_t permission[100];
			scanf("%ld ", &address);
			fgets(permission, 100, stdin);
			mprotect(arena, address, permission);
			ok = 1;
		}
		if (!ok)
			printf("Invalid command. Please try again.\n");
	}
	return 0;
}
