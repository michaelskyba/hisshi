#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "util.c"
#include "command.c"

void execute(struct command *cmd) {
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
		_exit(1);
	}

	printf("%d-%d: Starting wait()\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, id, status);

	clear_command(cmd);
}

void parse_script(FILE *script_file) {
	// TODO address lines longer than a fixed chunk_size 100
	char *name = (char *) malloc(100);
	char *p = name;

	int ln = 1;
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

		if (p == name && is_newline) {
			printf("%d: Skipping blank line\n", ln++);
			continue;
		}

		if (!cmd->path)
			cmd->path = get_bin_path(name);

		// Even if name == path, set $0 as convention
		add_arg(cmd, name);

		if (is_newline) {
			execute(cmd);
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
