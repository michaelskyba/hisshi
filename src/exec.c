// Returns exit code
int execute(struct command *cmd) {
	printf("exec start ");
	dump_command(cmd);

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
