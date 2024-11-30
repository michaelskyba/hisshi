#pragma once

#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37

// Used to represent
// - Internal shell variables
// - Environment variables
// - Functions
struct Binding {
	char *name;
	char *value;

	// This is part of a hash table, so we use LL to handle collision
	struct Binding *next;
};

int hash_str(char *str);

struct Binding *create_binding_struct(char *name, char *value);

void free_table(struct Binding **table);
void dump_table(struct Binding **table);

void set_table_binding(struct Binding **table, char *name, char *value);
char *get_table_binding(struct Binding **table, char *name);
int unset_table_binding(struct Binding **table, char *name);
