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
Variable *create_variable_struct(char *name, char *value) {
	Variable *var = malloc(sizeof(Variable));
	var->name = get_str_copy(name);
	var->value = get_str_copy(value);
	var->next = NULL;
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

// Returns whether we found and removed it
// 0: The variable didn't exist in the first place
int unset_table_variable(Variable **table, char *name) {
	int hash = hash_str(name);
	printf("Planning to unset %s (%d)\n", name, hash);

	Variable *prev = NULL;

	for (Variable *var = table[hash]; var != NULL; var = var->next) {
		printf("Found existing %s=%s on %d\n", var->name, var->value, hash);

		if (strcmp(var->name, name) == 0) {
			printf("%s=%s, so unsetting\n", var->name, name);

			if (prev == NULL)
				table[hash] = var->next;
			else
				prev->next = var->next;

			free(var->name);
			free(var->value);
			free(var);

			return 1;
		}

		prev = var;
	}

	return 0;
}

void load_env_vars(Variable **table) {
	// Automatically set by C
	extern char **environ;

	for (char **p = environ; *p != NULL; p++) {
		char *entry = *p;

		// It seems possible in oksh to at least set variables with equal signs
		// in their names, but we do not condone such a practice
		char *split = strchr(entry, '=');

		// To avoid having to clone the string twice, modify it temporarily
		// and then revert it, to avoid destroying the real entry
		*split = '\0';

		// Terminates at the \0 we just set
		char *name = entry;

		// Terminates at the original \0 that environ had for this entry
		char *value = split + 1;

		set_table_variable(table, name, value);

		// Revert
		*split = '=';
	}
}

ShellState *create_shell_state() {
	ShellState *state = malloc(sizeof(ShellState));
	state->shell_vars = calloc(HASH_BUCKETS, sizeof(Variable*));
	state->env_vars   = calloc(HASH_BUCKETS, sizeof(Variable*));
	load_env_vars(state->env_vars);

	return state;
}

void free_table(Variable **table) {
	Variable *next;

	for (int hash = 0; hash < HASH_BUCKETS; hash++) {
		if (table[hash] == NULL)
			continue;

		for (Variable *var = table[hash]; var != NULL; var = next) {
			next = var->next;

			free(var->name);
			free(var->value);
			free(var);
		}
	}

	free(table);
}

void free_shell_state(ShellState *state) {
	free_table(state->shell_vars);
	free_table(state->env_vars);
	free(state);
}

// Returns NULL rather than "" if not found
char *get_variable(ShellState *state, char *name) {
	char *val = get_table_variable(state->env_vars, name);
	if (!val)
		val = get_table_variable(state->shell_vars, name);

	return val;
}

void set_variable(ShellState *state, char *name, char *value) {
	if (get_table_variable(state->env_vars, name)) {
		// Update it externally so that when we copy environ in execve, it will
		// be reflected. The point of state->env_vars is for quick reading.
		int overwrite = 1;
		assert(setenv(name, value, overwrite) != -1);

		set_table_variable(state->env_vars, name, value);
		return;
	}

	set_table_variable(state->shell_vars, name, value);
}

void unset_variable(ShellState *state, char *name) {
	if (unset_table_variable(state->env_vars, name))
		assert(unsetenv(name) != -1);
	else
		unset_table_variable(state->shell_vars, name);
}
