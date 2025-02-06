#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command.h"
#include "parser.h"
#include "parse_state.h"
#include "shell_state.h"
#include "util.h"

#include "builtin.h"

typedef struct ArgNode ArgNode;
typedef struct Command Command;
typedef struct ParseState ParseState;
typedef struct ShellState ShellState;

/*
TODO: Implement a wrapper cd function to be included in the default config with
additional logic. Then make all builtins actually called "hisshi_cd" etc. to
avoid needing a "builtin" keyword. Then have default aliases for
exit=hisshi_exit etc. for those that are fine without a wrapper

Draft in /home/oboro/src/hisshi/rc/startup
*/
int builtin_cd(Command *cmd, ShellState *state) {
	// Technically mark as used
	(void) state;

	if (cmd->argc < 2) {
		fprintf(stderr, "builtin cd: no directory specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		fprintf(stderr, "builtin cd: multiple directories specified\n");
		return 1;
	}

	char *dir = cmd->arg_head->next->name;
	if (chdir(dir) != 0) {
		perror(dir);
		return 1;
	}

	return 0;
}

int builtin_exit(Command *cmd, ShellState *state) {
	// Technically mark as used
	(void) state;

	if (cmd->argc < 2) {
		fprintf(stderr, "builtin exit: no status specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		fprintf(stderr, "builtin exit: multiple codes given\n");
		return 1;
	}

	int status = atoi(cmd->arg_head->next->name);
	exit(status);

	// Shouldn't reach but we need a return value
	return 0;
}

int builtin_export(Command *cmd, ShellState *state) {
	// TODO Make docs describing exact design decisions and POSIX adherence. In
	// this case we are choosing not to support no args and multiple args,
	// as well as no `export foo=bar`

	// TODO Update those hypothetical docs. Right now 1738855839 I think that
	// support no args and multiple args is not any more work/code than the
	// current useless checks to enforce n=1.

	for (ArgNode *arg = cmd->arg_head->next; arg; arg = arg->next) {
		char *var_name = arg->name;
		export_variable(state, var_name);
	}

	return 0;
}

int builtin_unset(Command *cmd, ShellState *state) {
	for (ArgNode *arg = cmd->arg_head->next; arg; arg = arg->next) {
		char *var_name = arg->name;
		unset_variable(state, var_name);
	}

	return 0;
}

int builtin_eval(Command *cmd, ShellState *shell_state) {
	if (cmd->argc < 2) {
		fprintf(stderr, "builtin eval: no input specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		fprintf(stderr, "builtin eval: multiple inputs specified\n");
		return 1;
	}

	char *eval_body = copy_with_newline(cmd->arg_head->next->name);

	int pipe_fds[2];
	if (pipe(pipe_fds) == -1) {
		perror("pipe");
		free(eval_body);
		return 1;
	}

	ssize_t written = write(pipe_fds[1], eval_body, strlen(eval_body));
	if (written < 0) {
		perror("write");
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		free(eval_body);
		return 1;
	}
	close(pipe_fds[1]);

	FILE *source = fdopen(pipe_fds[0], "r");
	if (!source) {
		perror("fdopen");
		close(pipe_fds[0]);
		free(eval_body);
		return 1;
	}

	debug("Running parse script for body |%s|\n", eval_body);

	ParseState *parse_state = create_parse_state();
	parse_script(parse_state, shell_state, source);

	int exit_code = shell_state->exit_code;
	free_parse_state(parse_state);
	fclose(source);
	free(eval_body);

	return exit_code;
}

int (*get_builtin(char *name)) (Command *, ShellState *) {
	if (strcmp(name, "cd") == 0)
		return builtin_cd;

	if (strcmp(name, "exit") == 0)
		return builtin_exit;

	if (strcmp(name, "export") == 0)
		return builtin_export;

	if (strcmp(name, "unset") == 0)
		return builtin_unset;

	if (strcmp(name, "eval") == 0)
		return builtin_eval;

	return NULL;
}
