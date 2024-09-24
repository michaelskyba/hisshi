struct arg_node {
	char *name;
	struct arg_node *next;
};

struct command {
	char *path;

	int argc;
	struct arg_node *arg_head;
	struct arg_node *arg_tail;
};

char **get_argv_array(struct command *cmd) {
	// +1: Room for NULL
	int len = cmd->argc + 1;

	char **argv = (char **) malloc(sizeof(char *) * len);
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
	char *name = (char *) malloc(sizeof(char) * strlen(given_arg_name));
	strcpy(name, given_arg_name);

	struct arg_node *arg = (struct arg_node *) malloc(sizeof(struct arg_node));
	arg->name = name;
	arg->next = NULL;

	if (cmd->argc++ == 0)
		cmd->arg_head = arg;
	else
		cmd->arg_tail->next = arg;

	cmd->arg_tail = arg;
}

struct command *create_command() {
	struct command *cmd = (struct command *) malloc(sizeof(struct command));
	cmd->path = NULL;
	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	return cmd;
}

void dump_command(struct command *cmd) {
	if (!cmd->path) {
		printf("cmd %p (%d): %p\n", (void *) cmd->path, cmd->argc, (void *) cmd->arg_head);
		return;
	}

	printf("cmd %s (%d): ", cmd->path, cmd->argc);

	struct arg_node *arg;
	for (arg = cmd->arg_head; arg != NULL; arg = arg->next)
		printf("%s,", arg->name);

	printf("\n");
}

void clear_command(struct command *cmd) {
	// Already clear
	if (!cmd->path)
		return;

	struct arg_node *arg;
	struct arg_node *next;

	for (arg = cmd->arg_head; arg != NULL; arg = next) {
		printf("starting with arg %p, name %s\n", (void *) arg, arg->name);

		next = arg->next;

		printf("freeing arg->name\n");
		free(arg->name);

		printf("freeing arg\n");
		free(arg);
	}

	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	free(cmd->path);
	cmd->path = NULL;
}
