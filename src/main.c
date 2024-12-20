#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "hash_table.h"
#include "input_source.h"
#include "parser.h"
#include "parse_state.h"
#include "shell_state.h"
#include "util.h"

typedef struct InputSource InputSource;
typedef struct ParseState ParseState;
typedef struct ShellState ShellState;

void set_cli_args(ShellState *state, char argc, char **argv) {
	// 15: Assume we will have low digit counts of argc
	char *name = malloc(15);

	// POSIX only supports $1 to $9, but we provide freedom
	for (int i = 0; i < argc; i++) {
		sprintf(name, "%d", i);
		set_variable(state, name, argv[i]);
	}

	free(name);
}

int main(int argc, char **argv) {
	// TODO interactive usage (kak?)
	if (argc < 2) {
		fprintf(stderr, "Usage: hsh script-file\n");
		exit(1);
	}

	debug("Starting on PID %d\n", getpid());
	debug("%s: received file %s\n", argv[0], argv[1]);

	ParseState *parse_state = create_parse_state();
	ShellState *shell_state = create_shell_state(NULL);

	// 1 offset: We want the filename of the script to be $0, not the hsh binary
	// This is how regular shells do it
	set_cli_args(shell_state, argc-1, argv+1);

	FILE *script_file = fopen(argv[1], "r");
	assert(script_file);

	InputSource *source = create_file_input_source(script_file);
	parse_script(parse_state, shell_state, source);
	free_file_input_source(source);

	int exit_code = shell_state->exit_code;

	free_parse_state(parse_state);
	free_shell_state(shell_state);

	return exit_code;
}
