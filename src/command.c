struct arg_node_struct {
	char *name;
	struct arg_node_struct *next;
};
typedef struct arg_node_struct arg_node;

typedef struct {
	char *path;

	int argc;
	arg_node *arg_head;
	arg_node *arg_tail;

	// Used for determining control flow
	// Base: 0
	int indent_level;

	// Whether this command was run with a dash ("-"), indicating to only
	// run it if the previous on this indent failed
	// Besides setting this to true, the dash will be ignored as a token
	bool else_flag;
} command;

void clear_command(command *cmd) {
	cmd->indent_level = 0;
	cmd->else_flag = false;

	arg_node *arg;
	arg_node *next;

	for (arg = cmd->arg_head; arg != NULL; arg = next) {
		next = arg->next;

		free(arg->name);
		free(arg);
	}

	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	if (cmd->path) {
		free(cmd->path);
		cmd->path = NULL;
	}
}

command *create_command() {
	command *cmd = malloc(sizeof(command));

	// Avoid garbage values for pointers
	cmd->path = NULL;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	clear_command(cmd);
	return cmd;
}

char **get_argv_array(command *cmd) {
	// +1: Room for the NULL argument
	int len = cmd->argc + 1;

	char **argv = malloc(sizeof(char *) * len);
	arg_node *p = cmd->arg_head;

	for (int i = 0; i < cmd->argc; i++) {
		argv[i] = p->name;
		p = p->next;
	}

	argv[cmd->argc] = NULL;
	return argv;
}

// Creates a copy of the given name pointer
void add_arg(command *cmd, char *given_arg_name) {
	arg_node *arg = malloc(sizeof(arg_node));
	arg->name = get_str_copy(given_arg_name);
	arg->next = NULL;

	if (cmd->argc++ == 0)
		cmd->arg_head = arg;
	else
		cmd->arg_tail->next = arg;

	cmd->arg_tail = arg;
}

void dump_command(command *cmd) {
	if (!cmd->path) {
		printf("cmd %p (%d): %p\n", cmd->path, cmd->argc, (void *) cmd->arg_head);
		return;
	}

	printf("cmd %s (%d) at >%d: ", cmd->path, cmd->argc, cmd->indent_level);

	arg_node *arg;
	for (arg = cmd->arg_head; arg != NULL; arg = arg->next)
		printf("%s,", arg->name);

	printf("\n");
}
