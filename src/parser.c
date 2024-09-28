enum state_phase {
	command_name,
	command_arg,
};

struct parse_state {
	// Command we're constructing
	struct command *cmd;

	// The segment of the command we're constructing, like the path or an
	// argument
	char *token;

	// Whether we're reading a command name token or an argument
	int phase;

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

	state->phase = command_name;
	state->ln = 1;
	state->last_status = -1;

	return state;
}

void parse_script(FILE *script_file) {
	struct parse_state *state = create_state();
	char *p = state->token;

	while (true) {
		*p = getc(script_file);

		// Doesn't require any parsing because we declare that every file must
		// end with a \n
		if (*p == EOF)
			break;

		if (*p != ' ' && *p != '\n' && *p != '#') {
			p++;
			continue;
		}

		bool end_command = *p == '\n';

		// Delete the rest of the line for comments
		if (*p == '#') {
			while (getc(script_file) != '\n') ;
			end_command = true;
		}

		*p = '\0';

		// TODO support leading/trailing spaces
		// "Leading whitespace is unsupported, sorry :("
		// 1727479203: Actually maybe we can unironically say that

		if (p == state->token && state->phase == command_name && end_command) {
			printf("%d: Skipping blank line\n", state->ln++);
			printf("but token is %s\n", state->token);
			continue;
		}

		// Even if token == command name, set $0 as convention
		add_arg(state->cmd, state->token);

		if (state->phase == command_name) {
			state->cmd->path = get_bin_path(state->token);
			state->phase = command_arg;
		}

		if (end_command) {
			printf("Previous exit status: %d\n", state->last_status);
			state->last_status = execute(state->cmd);
			printf("Received new exit status: %d\n", state->last_status);

			state->ln++;
			state->phase = command_name;
		}

		p = state->token;
	}
}
