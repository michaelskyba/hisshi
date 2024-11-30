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

int builtin_export(Command *cmd, ShellState *state) {
	// TODO Make docs describing exact design decisions and POSIX adherence. In
	// this case we are choosing not to support no args and multiple args,
	// as well as no `export foo=bar`

	if (cmd->argc < 2) {
		printf("builtin export: no variable specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		printf("builtin export: multiple variables specified\n");
		return 1;
	}

	char *var_name = cmd->arg_head->next->name;
	export_variable(state, var_name);

	return 0;
}

int builtin_unset(Command *cmd, ShellState *state) {
	if (cmd->argc < 2) {
		printf("builtin unset: no variable specified\n");
		return 1;
	}

	if (cmd->argc > 2) {
		printf("builtin unset: multiple variables specified\n");
		return 1;
	}

	char *var_name = cmd->arg_head->next->name;
	unset_variable(state, var_name);

	return 0;
}

// int builtin_eval(Command *cmd, ShellState *shell_state) {
// 	if (cmd->argc < 2) {
// 		printf("builtin eval: no input specified\n");
// 		return 1;
// 	}

// 	if (cmd->argc > 2) {
// 		printf("builtin eval: multiple inputs specified\n");
// 		return 1;
// 	}

// 	char *eval_body = cmd->arg_head->next->name;
// 	InputSource *source = create_str_input_source(eval_body);

// 	ParseState *parse_state = create_parse_state();
// 	parse_script(parse_state, shell_state, source);

// 	int exit_code = shell_state->exit_code;
// 	free_parse_state(parse_state);
// 	free_str_input_source(source);

// 	return exit_code;
// }

int (*get_builtin(char *name)) (Command *, ShellState *) {
	if (strcmp(name, "cd") == 0)
		return builtin_cd;

	if (strcmp(name, "exit") == 0)
		return builtin_exit;

	if (strcmp(name, "export") == 0)
		return builtin_export;

	if (strcmp(name, "unset") == 0)
		return builtin_unset;

	// if (strcmp(name, "eval") == 0)
	// 	return builtin_eval;

	return NULL;
}
