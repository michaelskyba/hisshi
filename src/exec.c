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

int builtin_exit(Command *cmd) {
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

int (*get_builtin(char *name)) (Command *) {
	if (strcmp(name, "cd") == 0)
		return builtin_cd;

	if (strcmp(name, "exit") == 0)
		return builtin_exit;

	return NULL;
}

// Forks and returns child PID
// pipes: full list of pipes created for the whole pipeline, most of which each
// child should close
int execute_child(Command *cmd, int read_fd, int write_fd, int *pipes) {
	printf("execute_child (r%d --> w%d) start: ", read_fd, write_fd);
	dump_command(cmd);

	int pid = fork();

	if (pid < 0) {
		perror(cmd->path);
		exit(1);
	}

	if (pid > 0)
		return pid;

	for (int *fd = pipes; fd != NULL && *fd != -1; fd++)
		if (*fd != read_fd && *fd != write_fd)
			close(*fd);

	if (read_fd != STDIN_FILENO) {
		close(STDIN_FILENO);
		dup2(read_fd, STDIN_FILENO);
		close(read_fd);
	}

	if (write_fd != STDOUT_FILENO) {
		close(STDOUT_FILENO);
		dup2(write_fd, STDOUT_FILENO);
		close(write_fd);
	}

	int (*builtin)(Command *) = get_builtin(cmd->path);
	if (builtin) {
		int status = builtin(cmd);
		_exit(status);
	}

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

// foo | bar --> 2
int get_pipeline_length(Command *pipeline) {
	int i = 0;

	for (Command *cmd = pipeline; cmd != NULL; i++)
		cmd = cmd->next_pipeline;

	return i;
}

// Returns exit code of last command
int execute_pipeline(Command *pipeline) {
	Command *cmd = pipeline;
	int pipeline_length = get_pipeline_length(pipeline);

	if (pipeline_length == 1) {
		int (*builtin)(Command *) = get_builtin(cmd->path);
		if (builtin) {
			int status = builtin(cmd);
			return status;
		}

		// execute_child(cmd, STDIN_FILENO, STDOUT_FILENO, NULL);
		int pid = execute_child(cmd, STDIN_FILENO, STDOUT_FILENO, NULL);

		int status = 0;
		// wait(&status);

		int wait_pid = wait(&status);
		printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, wait_pid, status);

		return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
	}

	int num_fds = (pipeline_length-1) * 2;

	// Stores (r0, w0, r1, w1, ..., -1)
	// -1 to mark end, and not have to track length
	int *pipes = malloc(sizeof(int) * num_fds * 2 + 1);
	pipes[num_fds] = -1;

	for (int i = 0; i < pipeline_length-1; i++) {
		if (pipe(pipes + i*2) == -1) {
			perror("pipe() failed");
			assert(false);
		}

		printf("Opened pipe %d --> %d\n", pipes[i*2 + 1], pipes[i*2]);
	}

	for (int i = 0; i < pipeline_length; i++) {
		int r = i == 0 ? STDIN_FILENO : pipes[(i-1)*2];
		int w = i == pipeline_length - 1 ? STDOUT_FILENO : pipes[i*2 + 1];

		execute_child(cmd, r, w, pipes);
		if (r != STDIN_FILENO) close(r);
		if (w != STDOUT_FILENO) close(w);

		cmd = cmd->next_pipeline;
	}

	while (wait(NULL) > 0) ;
	return 0;

	// int pid = execute_child(cmd);
	// int pid = execute_child(cmd);

	// printf("%d-%d: Starting wait()\n", getpid(), pid);

	// int status = 0;
	// wait(&status);

	// int wait_pid = wait(&status);
	// printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, wait_pid, status);

	// return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
