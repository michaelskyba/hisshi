#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"
#include "shell_state.h"
#include "util.h"

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
	set_variable(state, "?", "0");

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

// If local, we shouldn't traverse up to parents in the function call stack when
// searching for values
bool is_local_binding(char *name) {
	if (strcmp(name, "0") == 0)
		return true;

	// <0: Not supported as a parameter
	// 0: Used as a default if name isn't an int. It can't be "0" because we
	// just checked for that
	if (atoi(name) > 0)
		return true;

	return name[0] == '_';
}

// Returns NULL rather than "" if not found
char *get_variable(ShellState *state, char *name) {
	char *val;

	val = get_table_binding(state->env_vars, name);
	if (val) return val;

	bool local = is_local_binding(name);

	while (state != NULL) {
		val = get_table_binding(state->shell_vars, name);
		if (val) return val;
		else if (local) break;

		state = state->parent;
		debug("shell var |%s| not found, so checking call stack parent %p\n", name, (void *) state);
	}

	debug("shell var |%s| not found in anywhere in the call stack\n", name);
	return NULL;
}

// If env var exists, overwrites it
// If name is local, always sets local
// If name isn't local and var exists in parent, overwrites parent
void set_variable(ShellState *state, char *name, char *value) {
	if (get_table_binding(state->env_vars, name)) {
		// Update it externally so that when we copy environ in execve, it will
		// be reflected. The point of state->env_vars is for quick reading.
		assert(setenv(name, value, SETENV_OVERWRITE) != -1);

		set_table_binding(state->env_vars, name, value);
		return;
	}

	ShellState *test_state = state;
	char *found_value = NULL;
	bool local = is_local_binding(name);

	while (!local && test_state != NULL) {
		found_value = get_table_binding(test_state->shell_vars, name);
		if (found_value) break;

		test_state = test_state->parent;
	}

	if (found_value) {
		debug("Found |%s| in parent, so overwriting there\n", name);
		state = test_state;
	}

	set_table_binding(state->shell_vars, name, value);
}

// - If the variable exists in env_vars, unset it there and finish.
// - If the binding is local, only unset it from the current state's shell_vars.
// - For non-local bindings, search the call stack (including parents) and unset
// the first occurrence found.
void unset_variable(ShellState *state, char *name) {
	if (get_table_binding(state->env_vars, name)) {
		unset_table_binding(state->env_vars, name);
		assert(unsetenv(name) != -1);
		return;
	}

	bool local = is_local_binding(name);

	if (local) {
		if (get_table_binding(state->shell_vars, name))
			unset_table_binding(state->shell_vars, name);
		return;
	}

	ShellState *target_state = state;
	while (target_state != NULL) {
		if (get_table_binding(target_state->shell_vars, name)) {
			unset_table_binding(target_state->shell_vars, name);
			return;
		}
		target_state = target_state->parent;
	}
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

// Never overwrites parent
void set_function(ShellState *state, char *name, char *body) {
	debug("Defining function |%s| --> |%s|\n", name, body);
	set_table_binding(state->functions, name, body);
}

char *get_function(ShellState *state, char *name) {
	// We don't need slashes in function names. This saves a search if the given
	// name param was already constructed with get_bin_path
	if (name[0] == '/')
		return NULL;

	char *body;
	bool local = is_local_binding(name);

	while (state != NULL) {
		body = get_table_binding(state->functions, name);
		if (body) return body;
		else if (local) break;

		state = state->parent;
		debug("func |%s| not found, so checking call stack parent %p\n", name, (void *) state);
	}

	debug("func |%s| not found anywhere in call stack\n", name);
	return NULL;
}

// Promote a function with the given name to global scope. Search the call stack
// for the first occurrence of the function in the functions hash tables. If
// found, remove every binding for that name in every scope and then set that
// binding in the global (root) ShellState.
void promote_function_to_global(ShellState *state, char *name) {
	ShellState *cur = state;
	char *body = NULL;

	ShellState *global = NULL;

	// Find first occurrence in the call stack.
	while (cur != NULL) {
		body = get_table_binding(cur->functions, name);
		if (body) break;

		global = cur;
		cur = cur->parent;
	}

	if (!body) return;

	// We will be keeping it to write again, so don't let unset_table_binding
	// clear
	body = get_str_copy(body);

	// Remove all function bindings for this name in every scope.
	while (cur != NULL) {
		if (get_table_binding(cur->functions, name))
			unset_table_binding(cur->functions, name);

		global = cur;
		cur = cur->parent;
	}

	set_table_binding(global->functions, name, body);
	free(body);
}

// Promote a shell variable with the given name to global scope. Search the call
// stack for the most local shell_vars binding with this name. If found, remove
// it from that scope and set it in the global scope.
void promote_variable_to_global(ShellState *state, char *name) {
	ShellState *cur = state;
	ShellState *global = state;
	char *value = NULL;

	while (cur != NULL) {
		value = get_table_binding(cur->shell_vars, name);
		if (value) break;

		global = state;
		cur = cur->parent;
	}

	if (!value) return;

	value = get_str_copy(value);
	ShellState *original = cur;

	while (global->parent != NULL)
		global = global->parent;

	if (original == global)
		return;

	set_table_binding(global->shell_vars, name, value);

	// Remove it from the original scope, also freeing value
	unset_table_binding(original->shell_vars, name);

}
