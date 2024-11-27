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
		printf("builtin cd: no directory specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		printf("builtin cd: multiple directories specified\n");
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
		printf("builtin exit: no status specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		printf("builtin exit: multiple codes given\n");
		return 1;
	}

	int status = atoi(cmd->arg_head->next->name);
	exit(status);

	// Shouldn't reach but we need a return value
	return 0;
}

int (*get_builtin(char *name)) (Command *, ShellState *) {
	if (strcmp(name, "cd") == 0)
		return builtin_cd;

	if (strcmp(name, "exit") == 0)
		return builtin_exit;

	return NULL;
}
