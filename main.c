#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void panic(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

void execute(char *cmd) {
	if (*cmd == '#') {
		printf("Skipping comment: %s", cmd);
		return;
	}

	if (*cmd == '\n') {
		printf("Skipping blank line\n");
		return;
	}

	printf("Executing %s on PID %d\n", cmd, getpid());
	system(cmd);
}

void run_script(FILE *script_file) {
	int chunk_size = 100;
	char line[chunk_size];

	while (fgets(line, chunk_size, script_file))
		execute(line);
}

int main(int argc, char **argv) {
	if (argc != 2)
		panic("Usage: hsh script-file");

	printf("Starting on PID %d\n", getpid());

	printf("%s: received file %s\n", argv[0], argv[1]);
	FILE *script_file = fopen(argv[1], "r");
	run_script(script_file);
}
