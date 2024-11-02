struct ArgNode_struct {
	char *name;
	struct ArgNode_struct *next;
};
typedef struct ArgNode_struct ArgNode;

struct Command_struct {
	char *path;

	// The arg list and argc include the $0 name
	int argc;
	ArgNode *arg_head;
	ArgNode *arg_tail;

	// Used for determining control flow
	// Base: 0
	int indent_level;

	// Whether this command was run with a dash ("-"), indicating to only
	// run it if the previous on this indent failed
	// Besides setting this to true, the dash will be ignored as a token
	bool else_flag;

	// The next command within the current pipeline
	// NULL if this is the last
	struct Command_struct *next_pipeline;

	// Point to a literal filename string if used. NULL otherwise
	char *redirect_read;
	char *redirect_write;
	char *redirect_append;
};
typedef struct Command_struct Command;

void clear_command(Command *cmd) {
	cmd->indent_level = 0;
	cmd->else_flag = false;

	ArgNode *arg;
	ArgNode *next;

	for (arg = cmd->arg_head; arg != NULL; arg = next) {
		next = arg->next;

		free(arg->name);
		free(arg);
	}

	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	if (cmd->next_pipeline) {
		clear_command(cmd->next_pipeline);
		cmd->next_pipeline = NULL;
	}

	// Strings
	if (cmd->path) {
		free(cmd->path);
		cmd->path = NULL;
	}
	if (cmd->redirect_read) {
		free(cmd->redirect_read);
		cmd->redirect_read = NULL;
	}
	if (cmd->redirect_write) {
		free(cmd->redirect_write);
		cmd->redirect_write = NULL;
	}
	if (cmd->redirect_append) {
		free(cmd->redirect_append);
		cmd->redirect_append = NULL;
	}
}

Command *create_command() {
	Command *cmd = malloc(sizeof(Command));

	// Avoid garbage values for pointers
	cmd->path = NULL;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;
	cmd->next_pipeline = NULL;
	cmd->redirect_read = NULL;
	cmd->redirect_write = NULL;
	cmd->redirect_append = NULL;

	clear_command(cmd);
	return cmd;
}

char **get_argv_array(Command *cmd) {
	// +1: Room for the NULL argument
	int len = cmd->argc + 1;

	char **argv = malloc(sizeof(char *) * len);
	ArgNode *p = cmd->arg_head;

	for (int i = 0; i < cmd->argc; i++) {
		argv[i] = p->name;
		p = p->next;
	}

	argv[cmd->argc] = NULL;
	return argv;
}

// Creates a copy of the given name pointer
void add_arg(Command *cmd, char *given_arg_name) {
	ArgNode *arg = malloc(sizeof(ArgNode));
	arg->name = get_str_copy(given_arg_name);
	arg->next = NULL;

	if (cmd->argc++ == 0)
		cmd->arg_head = arg;
	else
		cmd->arg_tail->next = arg;

	cmd->arg_tail = arg;
}

void dump_command(Command *cmd) {
	if (!cmd->path) {
		printf("cmd %p (%d): %p\n", cmd->path, cmd->argc, (void *) cmd->arg_head);
		return;
	}

	printf("cmd %s (%d) at >%d: ", cmd->path, cmd->argc, cmd->indent_level);

	ArgNode *arg;
	for (arg = cmd->arg_head; arg != NULL; arg = arg->next)
		printf("%s,", arg->name);

	printf("\n");
}
