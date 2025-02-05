#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hash_table.h"
#include "parser.h"
#include "parse_state.h"
#include "shell_state.h"
#include "tokenizer.h"
#include "util.h"

typedef struct ParseState ParseState;
typedef struct ShellState ShellState;
typedef struct Token Token;

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

	// Run initialization script if available
	char init_path[1024];
	FILE *init_file = NULL;
	const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
	const char *home = getenv("HOME");

	if (xdg_config_home) {
		snprintf(init_path, sizeof(init_path), "%s/hisshi/init", xdg_config_home);
		if (access(init_path, R_OK) == 0) {
			init_file = fopen(init_path, "r");
		}
	}
	if (!init_file && home) {
		snprintf(init_path, sizeof(init_path), "%s/.config/hisshi/init", home);
		if (access(init_path, R_OK) == 0) {
			init_file = fopen(init_path, "r");
		}
	}
	if (!init_file) {
		snprintf(init_path, sizeof(init_path), "/etc/hisshi_init");
		if (access(init_path, R_OK) == 0) {
			init_file = fopen(init_path, "r");
		}
	}
	if (init_file) {
		debug("Running init script: %s\n", init_path);
		parse_script(parse_state, shell_state, init_file);
		fclose(init_file);

		// It's unintuitive to track the init script as part of the line number
		parse_state->tk->ln = 1;
	}

	// 1 offset: We want the filename of the script to be $0, not the hsh binary
	// This is how regular shells do it
	set_cli_args(shell_state, argc - 1, argv + 1);

	FILE *script_file = fopen(argv[1], "r");
	assert(script_file);

	parse_script(parse_state, shell_state, script_file);
	fclose(script_file);

	int exit_code = shell_state->exit_code;

	free_parse_state(parse_state);
	free_shell_state(shell_state);

	return exit_code;
}
