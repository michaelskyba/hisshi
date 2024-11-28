#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37
#define SETENV_OVERWRITE 1

// Used to represent
// - Internal shell variables
// - Environment variables
// - Functions
struct binding_struct {
	char *name;
	char *value;

	// This is part of a hash table, so we use LL to handle collision
	struct binding_struct *next;
};
typedef struct binding_struct Binding;

typedef struct {
	/*
	We populate our own representation of env_vars at startup and then they're
	automatically synced. The only way for them to change is for a hisshi script
	to modify them or export a local variable, both of which we catch and
	setenv() accordingly. There's no way to source a C program etc. to run in
	the parent process.
	*/

	// Hash tables, size HASH_BUCKETS
	Binding **shell_vars;
	Binding **env_vars;
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
Binding *create_binding_struct(char *name, char *value) {
	Binding *bnd = malloc(sizeof(Binding));
	bnd->name = get_str_copy(name);
	bnd->value = get_str_copy(value);
	bnd->next = NULL;
	return bnd;
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
	printf("Setting %s (%d) to %s\n", name, hash, value);

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		printf("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			printf("%s=%s, so overwriting %s --> %s\n", bnd->name, name, bnd->value, value);

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
	printf("Searching for value of %s (%d)\n", name, hash);

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		printf("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			printf("%s=%s, so returning %s\n", bnd->name, name, bnd->value);
			return bnd->value;
		}
	}

	return NULL;
}

// Returns whether we found and removed it
// 0: The binding didn't exist in the first place
int unset_table_binding(Binding **table, char *name) {
	int hash = hash_str(name);
	printf("Planning to unset %s (%d)\n", name, hash);

	Binding *prev = NULL;

	for (Binding *bnd = table[hash]; bnd != NULL; bnd = bnd->next) {
		printf("Found existing %s=%s on %d\n", bnd->name, bnd->value, hash);

		if (strcmp(bnd->name, name) == 0) {
			printf("%s=%s, so unsetting\n", bnd->name, name);

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

void load_env_vars(Binding **table) {
	// Automatically set by C
	extern char **environ;

	for (char **p = environ; *p != NULL; p++) {
		char *entry = *p;

		// It seems possible in oksh to at least set bindings with equal signs
		// in their names, but we do not condone such a practice
		char *split = strchr(entry, '=');

		// To avoid having to clone the string twice, modify it temporarily
		// and then revert it, to avoid destroying the real entry
		*split = '\0';

		// Terminates at the \0 we just set
		char *name = entry;

		// Terminates at the original \0 that environ had for this entry
		char *value = split + 1;

		set_table_binding(table, name, value);

		// Revert
		*split = '=';
	}
}

ShellState *create_shell_state() {
	ShellState *state = malloc(sizeof(ShellState));
	state->shell_vars = calloc(HASH_BUCKETS, sizeof(Binding*));
	state->env_vars   = calloc(HASH_BUCKETS, sizeof(Binding*));
	load_env_vars(state->env_vars);

	return state;
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

void free_shell_state(ShellState *state) {
	free_table(state->shell_vars);
	free_table(state->env_vars);
	free(state);
}

// Returns NULL rather than "" if not found
char *get_variable(ShellState *state, char *name) {
	char *val = get_table_binding(state->env_vars, name);
	if (!val)
		val = get_table_binding(state->shell_vars, name);

	return val;
}

void set_variable(ShellState *state, char *name, char *value) {
	if (get_table_binding(state->env_vars, name)) {
		// Update it externally so that when we copy environ in execve, it will
		// be reflected. The point of state->env_vars is for quick reading.
		assert(setenv(name, value, SETENV_OVERWRITE) != -1);

		set_table_binding(state->env_vars, name, value);
		return;
	}

	set_table_binding(state->shell_vars, name, value);
}

void unset_variable(ShellState *state, char *name) {
	if (unset_table_binding(state->env_vars, name))
		assert(unsetenv(name) != -1);
	else
		unset_table_binding(state->shell_vars, name);
}

void export_variable(ShellState *state, char *name) {
	char *val = get_table_binding(state->shell_vars, name);

	// Either it doesn't exist at all, or it's already exported and thus not in
	// shell_vars
	if (val == NULL)
		return;

	// Set before unsetting to avoid having val cleared
	set_table_binding(state->env_vars, name, val);
	assert(setenv(name, val, SETENV_OVERWRITE) != -1);

	// Clears val too
	unset_table_binding(state->shell_vars, name);
}
