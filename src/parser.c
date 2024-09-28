struct parse_state {
	// Command we're constructing
	struct command *cmd;

	// The segment of the command we're constructing, like the path or an
	// argument
	char *token;

	// Whether we're reading a command name token (true) or an argument (false)
	bool reading_name;

	// Whether we've started receiving content for the current token (false),
	// or if we have nothing yet, either because we haven't asked or have
	// received spaces (true)
	bool waiting;

	// Line number
	int ln;

	/*
	A dynamic array storing most recent exit codes at different levels of
	indentation
	-1: invalid, represents a lack of record

	e.g. [1, 0, -1] means
	- A 1 exit code on the last command at the base level
	- A 0 exit code on the last command at one indent
	- No exit code recorded for the last command at two indents
	*/
	int *indent_exit_codes;
	int indents_tracked; // size
};

struct parse_state *create_state() {
	struct parse_state *state = malloc(sizeof(struct parse_state));
	state->cmd = create_command();

	// TODO address tokens longer than a fixed chunk_size of 100
	state->token = malloc(100);

	state->reading_name = true;
	state->waiting = true;
	state->ln = 1;

	state->indent_exit_codes = malloc(sizeof(int));
	*(state->indent_exit_codes) = -1;
	state->indents_tracked = 1;

	return state;
}

// Before parse_token(), we have just read an entire token and can now
// look at it, to place it inside state->cmd
void parse_token(struct parse_state *state) {
	// We just had empty spaces given, not a token
	if (state->waiting)
		return;

	printf("Reading name? (%d), but adding token |%s|\n", state->reading_name, state->token);

	// Even if token == command name, set $0 as convention
	add_arg(state->cmd, state->token);

	if (state->reading_name) {
		state->cmd->path = get_bin_path(state->token);
		state->reading_name = false;
	}
}

void update_status(struct parse_state *state, int status) {
	int indent = state->cmd->indent_level;
	state->indent_exit_codes[indent] = status;

	printf("Received >%d:%d\n", indent, status);

	if (indent+1 == state->indents_tracked) {
		state->indents_tracked *= 2;

		int size = sizeof(int) * state->indents_tracked;
		state->indent_exit_codes = realloc(state->indent_exit_codes, size);

		printf("reallocating indent tracker to size %d\n", state->indents_tracked);
	}
}

// The command is finished being read, so we execute it and update the parser
// state accordingly
void parse_command(struct parse_state *state) {
	// No command submitted
	if (state->reading_name && state->waiting)
		return;

	int indent = state->cmd->indent_level;

	if (indent > 0)
		printf("Last: >%d:%d\n", indent-1, state->indent_exit_codes[indent-1]);

	int success = 0;
	if (indent == 0 || state->indent_exit_codes[indent-1] == success) {
		int status = execute(state->cmd);
		update_status(state, status);
	}

	else printf("CF: skipping line %d\n", state->ln);

	state->ln++;
	state->reading_name = true;
	state->waiting = true;

	clear_command(state->cmd);
}

void parse_script(FILE *script_file) {
	struct parse_state *state = create_state();

	char *p = state->token;

	// Build up space/newline separated tokens, and then parse them
	// once constructed
	while (true) {
		*p = getc(script_file);

		// Doesn't require any parsing because we declare that every file must
		// end with a \n
		if (*p == EOF)
			break;

		// Otherwise, treat \t as a regular character in tokens
		if (*p == '\t' && state->reading_name && state->waiting) {
			state->cmd->indent_level++;
			continue;
		}

		// Delete the rest of the line for comments
		if (*p == '#') {
			while (getc(script_file) != '\n') ;
			*p = '\n';
		}

		if (*p != ' ' && *p != '\n') {
			state->waiting = false;

			p++;
			continue;
		}

		// We're now ending a token

		char sep = *p;
		*p = '\0';

		parse_token(state);

		if (sep == '\n')
			parse_command(state);

		if (sep == ' ')
			state->waiting = true;

		p = state->token;
	}
}
