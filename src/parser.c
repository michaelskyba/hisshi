struct parse_state {
	// Command we're constructing
	struct command *cmd;

	// The segment of the command we're constructing, like the path or an
	// argument
	char *token;

	// Line number
	int ln;

	// Exit code of previously run program on the top-level of indentation
	// Default: -1 (invalid and represents a lack of record)
	int last_status;
};

struct parse_state *create_state() {
	struct parse_state *state = malloc(sizeof(struct parse_state));
	state->cmd = create_command();

	// TODO address tokens longer than a fixed chunk_size of 100
	state->token = malloc(100);

	state->ln = 1;
	state->last_status = -1;

	return state;
}

void parse_script(FILE *script_file) {
	struct parse_state *state = create_state();
	char *p = state->token;

	while (1) {
		*p = getc(script_file);

		// Doesn't require any parsing because we declare that every file must
		// end with a \n
		if (*p == EOF)
			break;

		// TODO support trailing comments
		if (*p == '#' && p == state->token) {
			printf("%d: Skipping comment\n", state->ln++);
			while (getc(script_file) != '\n') ;
			continue;
		}

		if (*p != ' ' && *p != '\n') {
			p++;
			continue;
		}

		int is_newline = *p == '\n';
		*p = '\0';

		// TODO support leading/trailing spaces
		// "Leading whitespace is unsupported, sorry :("
		// 1727479203: Actually maybe we can unironically say that

		if (p == state->token && is_newline) {
			printf("%d: Skipping blank line\n", state->ln++);
			continue;
		}

		if (!state->cmd->path)
			state->cmd->path = get_bin_path(state->token);

		// Even if token == command name, set $0 as convention
		add_arg(state->cmd, state->token);

		if (is_newline) {
			printf("Previous exit status: %d\n", state->last_status);
			state->last_status = execute(state->cmd);
			printf("Received new exit status: %d\n", state->last_status);

			state->ln++;
		}

		p = state->token;
	}
}
