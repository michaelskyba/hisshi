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
