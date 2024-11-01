#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "util.c"
#include "command.c"
#include "shell_state.c"
#include "exec.c"
#include "tokenizer.c"
#include "parse_state.c"
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
	assert(script_file);
	parse_script(script_file);

	ShellState *state = create_shell_state();
	state->shell_vars[0] = create_variable_struct("key 1730495909", "value 1730495911");
	printf("%s\n", state->shell_vars[0]->name);
	printf("%s\n", state->shell_vars[0]->value);
}
