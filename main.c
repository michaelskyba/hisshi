#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

void panic(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

void execute(char *cmd) {
	int pid = fork();

	if (pid == 0) {
		char *argv[] = {NULL};
		char *env[] = {NULL};

		printf("%d: execve(%s)\n", getpid(), cmd);
		execve(cmd, argv, env);

		// execve only returns control to us if it fails
		perror(cmd);
		exit(1);
	}

	printf("%d-%d: Time to wait\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done waiting on %d with status %d\n", getpid(), pid, id, status);
}

void parse_line(char *line) {
	if (*line == '#') {
		printf("Skipping comment: %s", line);
		return;
	}

	if (*line == '\n') {
		printf("Skipping blank line\n");
		return;
	}

	// As of 1727056628 we're using fgets which pads a newline
	char *nl = strchr(line, '\n');
	if (nl) *nl = '\0';

	printf("Executing %s\n", line);
	execute(line);
}

void parse_script(FILE *script_file) {
	int chunk_size = 100;
	char *line = (char *) malloc(sizeof(char) * chunk_size);

	// while ((*line++ = getc(script_file)))
	// 	parse_line(line);

	while (fgets(line, chunk_size, script_file))
		parse_line(line);
}

int main(int argc, char **argv) {
	if (argc != 2)
		panic("Usage: hsh script-file");

	printf("Starting on PID %d\n", getpid());

	printf("%s: received file %s\n", argv[0], argv[1]);
	FILE *script_file = fopen(argv[1], "r");
	parse_script(script_file);
}
