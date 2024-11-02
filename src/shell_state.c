#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37

// Used to represent both internal shell variables and environment variables
struct variable_struct {
	char *name;
	char *value;

	// This is part of a hash table, so we use LL to handle collision
	struct variable_struct *next;
};
typedef struct variable_struct Variable;

typedef struct {
	/*
    We populate our own representation of env_vars at startup and then they're
	automatically synced. The only way for them to change is for a hisshi script
	to modify them or export a local variable, both of which we catch and
	setenv() accordingly. There's no way to source a C program etc. to run in
	the parent process.
	*/

	// Hash tables, size HASH_BUCKETS
	Variable **shell_vars;
	Variable **env_vars;
} ShellState;

ShellState *create_shell_state() {
	ShellState *state = malloc(sizeof(ShellState));
	state->shell_vars = malloc(sizeof(Variable*) * HASH_BUCKETS);
	state->env_vars   = malloc(sizeof(Variable*) * HASH_BUCKETS);

	return state;
}

int hash_str(char *str) {
	int hash_val = 0;
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		hash_val = hash_val * HASH_ROLL_CONSTANT + str[i];

	return hash_val % HASH_BUCKETS;
}

// Copies args
Variable *create_variable_struct(char *name, char *value) {
	Variable *var = malloc(sizeof(Variable));
	var->name = get_str_copy(name);
	var->value = get_str_copy(value);
	return var;
}

void dump_table(Variable **table) {
	bool empty = true;

	for (int hash = 0; hash < HASH_BUCKETS; hash++) {
		if (table[hash] == NULL)
			continue;

		empty = false;
		printf("table[%d]:\n", hash);

		for (Variable *var = table[hash]; var != NULL; var = var->next)
			printf("\t%s=|%s| --> %p\n", var->name, var->value, (void *) var->next);
	}

	if (empty)
		printf("(empty table)\n");
}

// Copies args
void set_table_variable(Variable **table, char *name, char *value) {
	int hash = hash_str(name);
	printf("Setting %s (%d) to %s\n", name, hash, value);

	for (Variable *var = table[hash]; var != NULL; var = var->next) {
		printf("Found existing %s=%s on %d\n", var->name, var->value, hash);

		if (strcmp(var->name, name) == 0) {
			printf("%s=%s, so overwriting %s --> %s\n", var->name, name, var->value, value);

			free(var->value);
			var->value = get_str_copy(value);
			return;
		}
	}

	// If none found, insert at start of list
	Variable *var = create_variable_struct(name, value);
	if (table[hash] != NULL)
		var->next = table[hash];

	table[hash] = var;
}

// Returns original, not copy
char *get_table_variable(Variable **table, char *name) {
	int hash = hash_str(name);
	printf("Searching for value of %s (%d)\n", name, hash);

	for (Variable *var = table[hash]; var != NULL; var = var->next) {
		printf("Found existing %s=%s on %d\n", var->name, var->value, hash);

		if (strcmp(var->name, name) == 0) {
			printf("%s=%s, so returning %s\n", var->name, name, var->value);
			return var->value;
		}
	}

	return NULL;
}
