#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

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
		char *env[] = {NULL};

		printf("%d: execve(%s)\n", getpid(), cmd->path);
		execve(cmd->path, get_argv_array(cmd), env);

		// execve only returns control to us if it fails
		perror(cmd->path);
		_exit(1);
	}

	printf("%d-%d: Starting wait()\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, id, status);

	clear_command(cmd);

	printf("end ");
	dump_command(cmd);
}

void parse_line(char *line) {
	if (*line == '\0') {
		printf("Skipping blank line\n");
		return;
	}

	if (*line == '#') {
		printf("Skipping comment: %s\n", line);
		return;
	}

	struct command *cmd = create_command();
	cmd->path = line;
	add_arg(cmd, cmd->path); // Convention: set name as $0

	// Test

	char *arg1 = (char *) malloc(2);
	arg1[0] = 'a';
	arg1[1] = 0;
	char *arg2 = (char *) malloc(2);
	arg2[0] = 'b';
	arg2[1] = 0;

	add_arg(cmd, arg1);
	add_arg(cmd, arg2);

	execute(cmd);
}

void parse_script(FILE *script_file) {
	// TODO address lines longer than chunk_size
	int chunk_size = 100;
	char *line = (char *) malloc(sizeof(char) * chunk_size);
	char *p = line;

	while ((*p = getc(script_file)) != EOF) {
		if (*p != '\n') {
			p++;
			continue;
		}

		*p = '\0';
		p = line;

		parse_line(line);
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
