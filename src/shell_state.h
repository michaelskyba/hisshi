#ifndef shell_state_h_INCLUDED
#define shell_state_h_INCLUDED

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

	// TODO scope system

	// Hash tables, size HASH_BUCKETS
	struct Binding **shell_vars;
	struct Binding **env_vars;

	struct Binding **functions;

	// Exit code of most recent command
	int exit_code;
};

void load_env_vars(struct Binding **table);

struct ShellState *create_shell_state();
void free_shell_state(struct ShellState *state);

char *get_variable(struct ShellState *state, char *name);
void set_variable(struct ShellState *state, char *name, char *value);
void unset_variable(struct ShellState *state, char *name);
void export_variable(struct ShellState *state, char *name);

void set_function(struct ShellState *state, char *name, char *body);
char *get_function(struct ShellState *state, char *name);

#endif // shell_state_h_INCLUDED
