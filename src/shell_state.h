#ifndef shell_state_h_INCLUDED
#define shell_state_h_INCLUDED

typedef struct Binding Binding; // hash_table.h

// ShellState does not refer to itself recursively but we want it to be named so
// that we can declare it as a named struct in other headers
typedef struct ShellState {
	/*
	We populate our own representation of env_vars at startup and then they're
	automatically synced. The only way for them to change is for a hisshi script
	to modify them or export a local variable, both of which we catch and
	setenv() accordingly. There's no way to source a C program etc. to run in
	the parent process.
	*/

	// TODO scope system

	// Hash tables, size HASH_BUCKETS
	Binding **shell_vars;
	Binding **env_vars;

	Binding **functions;

	// Exit code of most recent command
	int exit_code;
} ShellState;

void load_env_vars(Binding **table);

ShellState *create_shell_state();
void free_shell_state(ShellState *state);

char *get_variable(ShellState *state, char *name);
void set_variable(ShellState *state, char *name, char *value);
void unset_variable(ShellState *state, char *name);
void export_variable(ShellState *state, char *name);

void set_function(ShellState *state, char *name, char *body);
char *get_function(ShellState *state, char *name);

#endif // shell_state_h_INCLUDED
