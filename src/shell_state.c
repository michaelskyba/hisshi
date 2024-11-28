#define SETENV_OVERWRITE 1

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
