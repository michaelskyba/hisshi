#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "util.c"
#include "command.c"

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

		printf("%d: execve(%s)\n", getpid(), cmd->path);
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

	printf("%d-%d: Starting wait()\n", getpid(), pid);

	int status = 0;
	int wait_pid = wait(&status);
	printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, wait_pid, status);

	clear_command(cmd);
	return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

void parse_script(FILE *script_file) {
	// TODO address lines longer than a fixed chunk_size 100
	char *name = malloc(100);
	char *p = name;

	int ln = 1;
	int last_status = 0;

	struct command *cmd = create_command();

	while (1) {
		*p = getc(script_file);

		// Doesn't require any parsing because we declare that every file must
		// end with a \n
		if (*p == EOF)
			break;

		// TODO support trailing comments
		if (*p == '#' && p == name) {
			printf("%d: Skipping comment\n", ln++);
			while (getc(script_file) != '\n') ;
			continue;
		}

		if (*p != ' ' && *p != '\n') {
			p++;
			continue;
		}

		int is_newline = *p == '\n';
		*p = '\0';

		// TODO support leading/trailing spaces
		// "Leading whitespace is unsupported, sorry :("
		// 1727479203: Actually maybe we can unironically say that

		if (p == name && is_newline) {
			printf("%d: Skipping blank line\n", ln++);
			continue;
		}

		if (!cmd->path)
			cmd->path = get_bin_path(name);

		// Even if name == path, set $0 as convention
		add_arg(cmd, name);

		if (is_newline) {
			printf("Previous exit status: %d\n", last_status);
			last_status = execute(cmd);
			printf("Received new exit status: %d\n", last_status);

			ln++;
		}

		p = name;
	}
}

int main(int argc, char **argv) {
	// TODO interactive usage (kak?)
	if (argc != 2) {
		fprintf(stderr, "Usage: hsh script-file\n");
		exit(1);
	}

	printf("Starting on PID %d\n", getpid());
	printf("%s: received file %s\n", argv[0], argv[1]);

	FILE *script_file = fopen(argv[1], "r");
	parse_script(script_file);
}
