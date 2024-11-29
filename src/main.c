#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "util.c"
#include "command.c"
#include "hash_table.c"
#include "shell_state.c"
#include "builtin.c"
#include "exec.c"
#include "tokenizer.c"
#include "function.c"
#include "parse_state.c"
#include "parser.c"

void set_cli_args(Binding **table, char argc, char **argv) {
	// 15: Assume we will have low digit counts of argc
	char *name = malloc(15);

	for (int i = 0; i < argc; i++) {
		sprintf(name, "%d", i);
		set_table_binding(table, name, argv[i]);
	}

	free(name);
}

int main(int argc, char **argv) {
	// TODO interactive usage (kak?)
	if (argc < 2) {
		fprintf(stderr, "Usage: hsh script-file\n");
		exit(1);
	}

	printf("Starting on PID %d\n", getpid());
	printf("%s: received file %s\n", argv[0], argv[1]);

	ParseState *parse_state = create_parse_state();
	ShellState *shell_state = create_shell_state();

	// 1 offset: We want the filename of the script to be $0, not the hsh binary
	// This is how regular shells do it
	set_cli_args(shell_state->shell_vars, argc-1, argv+1);

	FILE *script_file = fopen(argv[1], "r");
	assert(script_file);
	parse_script(script_file, parse_state, shell_state);

	int exit_code = shell_state->exit_code;

	free_parse_state(parse_state);
	free_shell_state(shell_state);

	return exit_code;
}
