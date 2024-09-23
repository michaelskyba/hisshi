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

void add_arg(struct command *cmd, char *arg_name) {
	struct arg_node *arg = (struct arg_node *) malloc(sizeof(struct arg_node));
	arg->name = arg_name;
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
