#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

#include "builtin.h"
#include "command.h"
#include "exec.h"
#include "shell_state.h"

// We don't need these constants anywhere else so it's fine to have them in
// exec.c. Then we avoid having to include unistd.h in exec.h.

// How many bytes to read at a time, from a pipe into a char buffer
#define READ_PIPE_BYTES 4096

// -rw-r--r--
#define REDIR_CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct Command Command;
typedef struct ShellState ShellState;

// Forks and returns child PID
// pipes: full list of pipes created for the whole pipeline, most of which each
// child should close
// shell_state: children won't modify the parent state, but they can keep a copy
// of it, to modify and perform some logic surrounding it
int execute_child(Command *cmd, int read_fd, int write_fd, int *pipes, ShellState *shell_state) {
	int pid = fork();

	if (pid < 0) {
		perror(cmd->path);
		exit(1);
	}

	if (pid > 0)
		return pid;

	if (cmd->redirect_read) {
		read_fd = open(cmd->redirect_read, O_RDONLY);
		printf("read_fd override: file |%s| (%d)\n", cmd->redirect_read, read_fd);
		assert(read_fd != -1);
	}

	if (cmd->redirect_write) {
		int flags = O_WRONLY | O_CREAT | O_TRUNC;
		write_fd = open(cmd->redirect_write, flags, REDIR_CREATE_MODE);
		printf("write_fd write override: file |%s| (%d)\n", cmd->redirect_write, write_fd);
		assert(write_fd != -1);
	}
	else if (cmd->redirect_append) {
		int flags = O_WRONLY | O_CREAT | O_APPEND;
		write_fd = open(cmd->redirect_append, flags, REDIR_CREATE_MODE);
		printf("write_fd append override: file |%s| (%d)\n", cmd->redirect_append, write_fd);
		assert(write_fd != -1);
	}

	printf("execute_child (r%d --> w%d) start: ", read_fd, write_fd);
	dump_command(cmd);

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

	int (*builtin)(Command *, ShellState *) = get_builtin(cmd->path);
	if (builtin) {
		int status = builtin(cmd, shell_state);
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
// |= does not count as an extra command in the pipeline
int get_pipeline_length(Command *pipeline) {
	int i = 0;

	for (Command *cmd = pipeline; cmd != NULL; cmd = cmd->next_pipeline)
		i++;

	return i;
}

char *read_pipe_var(int read_fd) {
	int last_bytes_read = 0;

	// buf's size should always be a multiple of how much we're reading
	int buf_size = READ_PIPE_BYTES;
	char *buf = malloc(buf_size+1);
	char *p = buf;

	while ((last_bytes_read = read(read_fd, p, READ_PIPE_BYTES)) > 0) {
		p += last_bytes_read;

		if (p - buf == buf_size) {
			buf_size *= 2;
			buf = realloc(buf, buf_size+1);

			// Return p to where it was before the realloc
			p = buf + buf_size/2;
		}
	}

	if (last_bytes_read == -1)
		perror("read_pipe_var");

	*p = '\0';

	// Crop trailing \n
	p--;
	if (p > buf && *p == '\n')
		*p = '\0';

	return buf;
}

// Returns exit code of last command
int execute_pipeline(Command *pipeline, ShellState *shell_state) {
	Command *cmd = pipeline;
	int pipeline_length = get_pipeline_length(pipeline);

	int last_pid;
	int *pipe_variable_fds;
	int last_write_fd = STDOUT_FILENO;

	Command *last_cmd = pipeline;
	while (last_cmd->next_pipeline != NULL)
		last_cmd = last_cmd->next_pipeline;

	// This pipeline ends with a |=
	if (last_cmd->pipe_variable) {
		pipe_variable_fds = malloc(sizeof(int) * 2);
		if (pipe(pipe_variable_fds) == -1) {
			perror("pipe() failed");
			assert(false);
		}

		last_write_fd = pipe_variable_fds[1];
		printf("|= pipe: %d --> %d\n", pipe_variable_fds[1], pipe_variable_fds[0]);
	}

	if (pipeline_length == 1) {
		int (*builtin)(Command *, ShellState *) = get_builtin(cmd->path);
		if (builtin) {
			int status = builtin(cmd, shell_state);
			return status;
		}

		int *pipes = NULL;
		if (last_cmd->pipe_variable) {
			pipes = malloc(sizeof(int) * 3);

			// Include read end so the child closes it
			pipes[0] = pipe_variable_fds[0];

			pipes[1] = pipe_variable_fds[1];
			pipes[2] = -1;
		}

		last_pid = execute_child(cmd, STDIN_FILENO, last_write_fd, pipes, shell_state);

		// Don't close read because we will actually use it later. Only children
		// close it
		if (last_cmd->pipe_variable)
			close(pipe_variable_fds[1]);

		free(pipes);
	}

	else {
		int num_fds = (pipeline_length-1) * 2;

		// Stores (r0, w0, r1, w1, ..., -1)
		// -1 to mark end, and not have to track length
		int *pipes = malloc(sizeof(int) * (num_fds + 1));
		pipes[num_fds] = -1;

		for (int i = 0; i < pipeline_length-1; i++) {
			if (pipe(pipes + i*2) == -1) {
				perror("pipe() failed");
				assert(false);
			}

			printf("Opened pipe %d --> %d\n", pipes[i*2 + 1], pipes[i*2]);
		}

		// Not read/written to directly from this pipes array, but still
		// included so that each execute_child closes them
		if (last_cmd->pipe_variable) {
			pipes = realloc(pipes, sizeof(int) * (num_fds + 1 + 2));
			pipes[num_fds] = pipe_variable_fds[0];
			pipes[num_fds+1] = pipe_variable_fds[1];
			pipes[num_fds+2] = -1;
		}

		for (int i = 0; i < pipeline_length; i++) {
			int r = i == 0 ? STDIN_FILENO : pipes[(i-1)*2];
			int w = i == pipeline_length - 1 ? last_write_fd : pipes[i*2 + 1];

			last_pid = execute_child(cmd, r, w, pipes, shell_state);

			if (r != STDIN_FILENO) close(r);
			if (w != STDOUT_FILENO) close(w);

			cmd = cmd->next_pipeline;
		}

		free(pipes);
	}

	int wait_pid;
	int status;
	int last_status = 0;

	while ((wait_pid = wait(&status)) > 0)
		if (wait_pid == last_pid)
			last_status = status;

	if (last_cmd->pipe_variable) {
		char *name = last_cmd->pipe_variable;
		int read_fd = pipe_variable_fds[0];

		char *value = read_pipe_var(read_fd);
		printf("Received value |%s| from fd %d. Placing into var |%s|\n", value, read_fd, name);

		// Clones value
		set_variable(shell_state, name, value);
		free(value);

		// The write end has already been closed by us (parent) earlier
		close(read_fd);
		free(pipe_variable_fds);
	}

	return WIFEXITED(last_status) ? WEXITSTATUS(last_status) : 1;
}
