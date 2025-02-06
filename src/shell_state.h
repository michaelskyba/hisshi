#pragma once

#include "stdbool.h" // for is_local_binding

struct Binding; // hash_table.h

// ShellState does not refer to itself recursively but we want it to be named so
// that we can declare it as a named struct in other headers
struct ShellState {
	/*
	We populate our own representation of env_vars at startup and then they're
	automatically synced. The only way for them to change is for a hisshi script
	to modify them or export a local variable, both of which we catch and
	setenv() accordingly. There's no way to source a C program etc. to run in
	the parent process.
	*/

	// Binding ** are hash tables, of size HASH_BUCKETS

	// This same pointer is passed to every constructed ShellState,
	// since env vars are all shared
	struct Binding **env_vars;

	// Local to each ShellState in the function call stack
	struct Binding **shell_vars;
	struct Binding **functions;

	// Exit code of most recent command
	int exit_code;

	// Used to implement a ShellState stack for function calls. The root root
	// global scope has parent == NULL.
	// For now we have no use case for traversing to children given a parent, so
	// we don't need it doubly linked
	struct ShellState *parent;
};

void load_env_vars(struct Binding **table);

struct ShellState *create_shell_state(struct ShellState *parent);
void free_shell_state(struct ShellState *state);

bool is_local_binding(char *name);

char *get_variable(struct ShellState *state, char *name);
void set_variable(struct ShellState *state, char *name, char *value);
void unset_variable(struct ShellState *state, char *name);
void export_variable(struct ShellState *state, char *name);

void set_function(struct ShellState *state, char *name, char *body);
char *get_function(struct ShellState *state, char *name);

void promote_function_to_global(struct ShellState *state, char *name);
void promote_variable_to_global(struct ShellState *state, char *name);
