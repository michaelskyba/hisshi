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

	if (pid < 0) {
		perror(cmd);
		exit(1);
	}

	if (pid == 0) {
		char *argv[] = {NULL};
		char *env[] = {NULL};

		printf("%d: execve(%s)\n", getpid(), cmd);
		execve(cmd, argv, env);

		// execve only returns control to us if it fails
		perror(cmd);
		_exit(1);
	}

	printf("%d-%d: Time to wait\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done waiting on %d with status %d\n", getpid(), pid, id, status);

	printf("cmd is %s\n", cmd);
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

	printf("Executing %s\n", line);
	execute(line);
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
		parse_line(line);
		p = line;
	}
}

int main(int argc, char **argv) {
	if (argc != 2)
		panic("Usage: hsh script-file");

	printf("Starting on PID %d\n", getpid());
	printf("%s: received file %s\n", argv[0], argv[1]);

	FILE *script_file = fopen(argv[1], "r");
	parse_script(script_file);
}
