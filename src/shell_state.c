#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"
#include "shell_state.h"

#define SETENV_OVERWRITE 1

typedef struct Binding Binding;
typedef struct ShellState ShellState;

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

// parent is in the context of the ShellState function call stack. Pass NULL if
// this is the main.c root
// env_vars are either copied or created depending on NULL
ShellState *create_shell_state(ShellState *parent) {
	ShellState *state = malloc(sizeof(ShellState));

	if (parent) {
		state->parent = parent;
		state->env_vars = parent->env_vars;
	}
	else {
		state->parent = NULL;
		state->env_vars = calloc(HASH_BUCKETS, sizeof(Binding*));
		load_env_vars(state->env_vars);
	}

	state->shell_vars = calloc(HASH_BUCKETS, sizeof(Binding*));
	state->functions  = calloc(HASH_BUCKETS, sizeof(Binding*));

	// $? starts at 0 in shells by convention
	state->exit_code = 0;

	return state;
}

void free_shell_state(ShellState *state) {
	// If there's a parent, it's going to use the same env_vars and thus should
	// be left alone
	if (!state->parent)
		free_table(state->env_vars);

	free_table(state->shell_vars);
	free_table(state->functions);
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

void set_function(ShellState *state, char *name, char *body) {
	printf("Defining function |%s| --> |%s|\n", name, body);
	set_table_binding(state->functions, name, body);
}

char *get_function(ShellState *state, char *name) {
	return get_table_binding(state->functions, name);
}
