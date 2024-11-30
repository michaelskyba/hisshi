#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "util.h"
#include "command.h"

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
		free(cmd->next_pipeline);
		cmd->next_pipeline = NULL;
	}

	// Strings
	if (cmd->path) {
		free(cmd->path);
		cmd->path = NULL;
	}
	if (cmd->pipe_variable) {
		free(cmd->pipe_variable);
		cmd->pipe_variable = NULL;
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
	cmd->pipe_variable = NULL;
	cmd->redirect_read = NULL;
	cmd->redirect_write = NULL;
	cmd->redirect_append = NULL;

	clear_command(cmd);
	return cmd;
}

void free_command(Command *cmd) {
	clear_command(cmd);
	free(cmd);
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
