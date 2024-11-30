#ifndef hash_table_h_INCLUDED
#define hash_table_h_INCLUDED

#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37

// Used to represent
// - Internal shell variables
// - Environment variables
// - Functions
typedef struct Binding {
	char *name;
	char *value;

	// This is part of a hash table, so we use LL to handle collision
	struct Binding *next;
} Binding;

int hash_str(char *str);

Binding *create_binding_struct(char *name, char *value);

void free_table(Binding **table);
void dump_table(Binding **table);

void set_table_binding(Binding **table, char *name, char *value);
char *get_table_binding(Binding **table, char *name);
int unset_table_binding(Binding **table, char *name);

#endif // hash_table_h_INCLUDED
