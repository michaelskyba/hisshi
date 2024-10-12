// indent_controls options, governing tracking of control flow
enum {
	// Default: no branch of this control flow structure has matched. elifs and
	// elses can still activate.
	control_waiting,

	// The current branch that we're parsing commands within was matched and is
	// active. The body should execute.
	control_branch_active,

	// The current branch is inactive, since we've matched a previous branch
	// within the same control structure. Any new bodies should not execute.
	control_complete,
};

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
	A dynamic array storing the control flow statuses at different levels of
	indentation
	[n]: status at n levels of indentation (base starting at 0)
	*/
	int *indent_controls;
	int indents_tracked; // total allocated room
};

struct parse_state *create_state() {
	struct parse_state *state = malloc(sizeof(struct parse_state));
	state->cmd = create_command();

	// TODO address tokens longer than a fixed chunk_size of 100
	state->token = malloc(100);

	state->reading_name = true;
	state->waiting = true;
	state->ln = 1;

	state->indent_controls = malloc(sizeof(int));
	*(state->indent_controls) = control_waiting;
	state->indents_tracked = 1;

	return state;
}

// Before parse_token(), we have just read an entire token and can now
// look at it, to place it inside state->cmd
void parse_token(struct parse_state *state) {
	// We just had empty spaces given, not a token
	if (state->waiting)
		return;

	if (state->reading_name && strcmp(state->token, "-") == 0) {
		state->cmd->else_flag = true;
		return;
	}

	printf("Reading name? (%d), but adding token |%s|\n", state->reading_name, state->token);

	// Even if token == command name, set $0 as convention
	add_arg(state->cmd, state->token);

	if (state->reading_name) {
		state->cmd->path = get_bin_path(state->token);
		state->reading_name = false;
	}
}

char *control_name(int control) {
	char **names = malloc(3 * sizeof(char *));
	names[control_waiting] = "waiting";
	names[control_branch_active] = "active";
	names[control_complete] = "complete";

	return names[control];
}

void update_control(struct parse_state *state, int status) {
	int indent = state->cmd->indent_level;
	state->indent_controls[indent] = status;

	printf("Received >%d:%s\n", indent, control_name(status));

	if (indent+1 == state->indents_tracked) {
		state->indents_tracked *= 2;

		int size = sizeof(int) * state->indents_tracked;
		state->indent_controls = realloc(state->indent_controls, size);

		printf("reallocating indent tracker to size %d\n", state->indents_tracked);
	}
}

// The command is finished being read, so we examine its context within the
// control flow structure and potentially execute it
void parse_command(struct parse_state *state) {
	int indent = state->cmd->indent_level;

	// TODO: If a branch shouldn't be executed, all parsing of it should be
	// skipped. Both for performance and because we don't want to evaluate
	// subshells etc. in there

	// No command submitted
	if (state->reading_name && state->cmd->path == NULL) {
		// Blank "-\n", equivalent to "- true\n"
		if (state->cmd->else_flag && state->indent_controls[indent] == control_waiting)
			update_control(state, control_branch_active);

		state->ln++;
		state->reading_name = true;
		state->waiting = true;

		clear_command(state->cmd);
		return;
	}

	printf("reading name? %d | waiting? %d\n", state->reading_name, state->waiting);

	int parent_control = indent == 0 ? control_branch_active : state->indent_controls[indent-1];
	int control = state->indent_controls[indent];
	printf(">%d:%s, >%d:%s\n", indent-1, control_name(parent_control), indent, control_name(control));

	bool parent_permits = parent_control == control_branch_active;

	// We're not supposed to be executing because the previous branch was active
	if (state->cmd->else_flag && control == control_branch_active)
		update_control(state, control_complete);

	if (parent_permits && (!state->cmd->else_flag || control == control_waiting)) {
		int exit_code = execute(state->cmd);

		// 0: success exit code, so this if branch is now active
		if (exit_code == 0)
			update_control(state, control_branch_active);

		// The command failed, but this is a new start of a control structure,
		// since it's a base (if). We don't execute this body but we allow
		// further branches to check their conditions
		else if (!state->cmd->else_flag)
			update_control(state, control_waiting);

	}

	else printf("CF: skipping execution of line %d\n", state->ln);

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
