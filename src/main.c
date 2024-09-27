#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "util.c"
#include "command.c"
#include "exec.c"
#include "parser.c"

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
