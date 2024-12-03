#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"
#include "util.h"

typedef struct Binding Binding;

int hash_str(char *str) {
	long hash_val = 0;
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		hash_val = hash_val * HASH_ROLL_CONSTANT + str[i];

	hash_val = hash_val % HASH_BUCKETS;

	// If it rolled over already and is negative, modulo still works well enough,
	// except -x % y = -(x % y)
	if (hash_val < 0)
		hash_val += HASH_BUCKETS;

	return hash_val;
}

// Copies args
Binding *create_binding_struct(char *name, char *value) {
	Binding *bnd = malloc(sizeof(Binding));
	bnd->name = get_str_copy(name);
	bnd->value = get_str_copy(value);
	bnd->next = NULL;
	return bnd;
}

void free_table(Binding **table) {
	Binding *next;

	for (int hash = 0; hash < HASH_BUCKETS; hash++) {
		if (table[hash] == NULL)
			continue;

		for (Binding *bnd = table[hash]; bnd != NULL; bnd = next) {
			next = bnd->next;

			free(bnd->name);
			free(bnd->value);
			free(bnd);
		}
	}

	free(table);
}

void dump_table(Binding **table) {
	bool empty = true;

	for (int hash = 0; hash < HASH_BUCKETS; hash++) {
		if (table[hash] == NULL)
			continue;

		empty = false;
		printf("table[%d]:\n", hash);

		for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next)
			printf("\t%s=|%s| --> %p\n", bnd->name, bnd->value, (void *) bnd->next);
	}

	if (empty)
		printf("(empty table)\n");
}

// Copies args
void set_table_binding(Binding **table, char *name, char *value) {
	int hash = hash_str(name);
	debug("Setting %s (%d) to %s\n", name, hash, value);

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		debug("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			debug("%s=%s, so overwriting %s --> %s\n", bnd->name, name, bnd->value, value);

			free(bnd->value);
			bnd->value = get_str_copy(value);
			return;
		}
	}

	// If none found, insert at start of list
	Binding *bnd = create_binding_struct(name, value);
	if (table[hash] != NULL)
		bnd->next = table[hash];

	table[hash] = bnd;
}

// Returns original, not copy
char *get_table_binding(Binding **table, char *name) {
	int hash = hash_str(name);
	debug("Searching for value of %s (%d)\n", name, hash);

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		debug("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			debug("%s=%s, so returning %s\n", bnd->name, name, bnd->value);
			return bnd->value;
		}
	}

	return NULL;
}

// Returns whether we found and removed it
// 0: The binding didn't exist in the first place
int unset_table_binding(Binding **table, char *name) {
	int hash = hash_str(name);
	debug("Planning to unset %s (%d)\n", name, hash);

	Binding *prev = NULL;

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		debug("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			debug("%s=%s, so unsetting\n", bnd->name, name);

			if (prev == NULL)
				table[hash] = bnd->next;
			else
				prev->next = bnd->next;

			free(bnd->name);
			free(bnd->value);
			free(bnd);

			return 1;
		}

		prev = bnd;
	}

	return 0;
}
