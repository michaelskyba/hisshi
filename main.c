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
		exit(1);
	}

	printf("%d-%d: Time to wait\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done waiting on %d with status %d\n", getpid(), pid, id, status);

	printf("cmd is %s\n", cmd);
}

void parse_line(char *line) {
	printf("Starting parse_line: %s\n", line);

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
	// printf("Using file with fd %d\n", fileno(script_file));

	int chunk_size = 100;
	char *line = (char *) malloc(sizeof(char) * chunk_size);
	char *tmp = (char *) malloc(sizeof(char) * chunk_size);

	int lines = 0;

	// while ((*line++ = getc(script_file)))
	// 	parse_line(line);

	while ((tmp = fgets(line, chunk_size, script_file))) {
		lines++;
		printf("Starting inner fgets loop with pos %ld | tmp %s | line %s\n", ftell(script_file), tmp, line);
		printf("We have just read line %d\n", lines);

		if (lines > 10) {
			printf("Exiting because that's too much\n");
			exit(1);
		}

		printf("before offset %ld\n", lseek(3, 0, SEEK_CUR));
		parse_line(line);
		printf("after offset %ld\n", lseek(3, 0, SEEK_CUR));

		printf("current offset %ld\n", lseek(3, 0, SEEK_CUR));
		printf("pos aftewards: %ld\n", ftell(script_file));
	}

	printf("Done since tmp was %s\n", tmp);

	// printf("current file pos: %ld\n", ftell(script_file));
	// tmp = fgets(line, chunk_size, script_file);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("0. done fgets, got tmp %s\n", tmp);
	// parse_line(line);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("1. done line, is now %s\n", line);

	// tmp = fgets(line, chunk_size, script_file);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("2. done fgets, got tmp %s\n", tmp);
	// parse_line(line);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("3. done line, is now %s\n", line);

	// tmp = fgets(line, chunk_size, script_file);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("4. done fgets, got tmp %s\n", tmp);
	// parse_line(line);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("5. done line, is now %s\n", line);

	// tmp = fgets(line, chunk_size, script_file);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("6. done fgets, got tmp %s\n", tmp);
	// parse_line(line);
	// printf("current file pos: %ld\n", ftell(script_file));
	// printf("7. done line, is now %s\n", line);
}

int main(int argc, char **argv) {
	if (argc != 2)
		panic("Usage: hsh script-file");

	printf("Starting on PID %d\n", getpid());
	printf("%s: received file %s\n", argv[0], argv[1]);

	FILE *script_file = fopen(argv[1], "r");
	parse_script(script_file);
}
