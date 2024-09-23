#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void panic(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

void execute(char *cmd) {
	int pid = fork();

	if (pid == 0) {
		char *argv[] = {NULL};
		char *env[] = {NULL};

		printf("I'm the child %d, so I'm execve() ing\n", getpid());
		int status = execve(cmd, argv, env);
		printf("Child finished with status %d...\n", status);
		return;
	}

	printf("I'm the parent (%d), who created %d\n", getpid(), pid);
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
