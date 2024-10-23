/*
TODO: Implement a wrapper cd function to be included in the default config with
additional logic. Then make all builtins actually called "hisshi_cd" etc. to
avoid needing a "builtin" keyword. Then have default aliases for
exit=hisshi_exit etc. for those that are fine without a wrapper

Draft in /home/oboro/src/hisshi/rc/startup
*/
int builtin_cd(Command *cmd) {
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

// Returns exit code
int execute(Command *cmd) {
	printf("exec start ");
	dump_command(cmd);

	if (strcmp(cmd->path, "cd") == 0)
		return builtin_cd(cmd);

	int pid = fork();

	if (pid < 0) {
		perror(cmd->path);
		exit(1);
	}

	if (pid == 0) {
		// Current system env variables
		extern char **environ;

		// printf("%d: execve(%s)\n", getpid(), cmd->path);
		execve(cmd->path, get_argv_array(cmd), environ);

		// execve only returns control to us if it fails
		perror(cmd->path);

		// Not found
		if (errno == ENOENT)
			_exit(127);

		// No access to executing it
		else if (errno == EACCES)
			_exit(126);

		_exit(1);
	}

	// printf("%d-%d: Starting wait()\n", getpid(), pid);

	int status = 0;
	wait(&status);

	// int wait_pid = wait(&status);
	// printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, wait_pid, status);

	return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
