struct arg_node {
	char *name;
	struct arg_node *next;
};

struct command {
	char *path;

	int argc;
	struct arg_node *arg_head;
	struct arg_node *arg_tail;

	// Used for determining control flow
	// Base: 0
	int indent_level;

	// Whether this command was run with a dash ("-"), indicating to only
	// run it if the previous on this indent failed
	// Besides setting this to true, the dash will be ignored as a token
	bool else_flag;
};

struct command *create_command() {
	struct command *cmd = malloc(sizeof(struct command));

	cmd->path = NULL;
	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	cmd->indent_level = 0;
	cmd->else_flag = false;

	return cmd;
}

char **get_argv_array(struct command *cmd) {
	// +1: Room for the NULL argument
	int len = cmd->argc + 1;

	char **argv = malloc(sizeof(char *) * len);
	struct arg_node *p = cmd->arg_head;

	for (int i = 0; i < cmd->argc; i++) {
		argv[i] = p->name;
		p = p->next;
	}

	argv[cmd->argc] = NULL;
	return argv;
}

// Creates a copy of the given name pointer
void add_arg(struct command *cmd, char *given_arg_name) {
	struct arg_node *arg = malloc(sizeof(struct arg_node));
	arg->name = get_str_copy(given_arg_name);
	arg->next = NULL;

	if (cmd->argc++ == 0)
		cmd->arg_head = arg;
	else
		cmd->arg_tail->next = arg;

	cmd->arg_tail = arg;
}

void dump_command(struct command *cmd) {
	if (!cmd->path) {
		printf("cmd %p (%d): %p\n", cmd->path, cmd->argc, (void *) cmd->arg_head);
		return;
	}

	printf("cmd %s (%d) at >%d: ", cmd->path, cmd->argc, cmd->indent_level);

	struct arg_node *arg;
	for (arg = cmd->arg_head; arg != NULL; arg = arg->next)
		printf("%s,", arg->name);

	printf("\n");
}

void clear_command(struct command *cmd) {
	printf("Clearing state->cmd\n");

	struct arg_node *arg;
	struct arg_node *next;

	for (arg = cmd->arg_head; arg != NULL; arg = next) {
		next = arg->next;

		free(arg->name);
		free(arg);
	}

	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	cmd->indent_level = 0;
	cmd->else_flag = false;

	if (cmd->path) {
		free(cmd->path);
		cmd->path = NULL;
	}
}
