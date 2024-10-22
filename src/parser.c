// For parse_state.indent_controls
enum {
	// Default: no branch of this control flow structure has matched. elifs and
	// elses can still activate.
	CONTROL_WAITING,

	// The current branch that we're parsing commands within was matched and is
	// active. The body should execute.
	CONTROL_BRANCH_ACTIVE,

	// The current branch is inactive, since we've matched a previous branch
	// within the same control structure. Any new bodies should not execute.
	CONTROL_COMPLETE,
};

// For parse_state.phase
enum {
	READING_INDENTS,
	READING_NAME,
	READING_ARG,
};

typedef struct {
	// Command we're constructing
	command *cmd;

	// The segment of the command we're constructing, like the path or an
	// argument
	char *token;

	// Which part of the line we're parsing
	int phase;

	// Line number
	int ln;

	/*
	A dynamic array storing the control flow statuses at different levels of
	indentation
	[n]: status at n levels of indentation (base starting at 0)
	*/
	int *indent_controls;
	int indents_tracked; // total allocated room
} parse_state;

parse_state *create_state() {
	parse_state *state = malloc(sizeof(parse_state));
	state->cmd = create_command();

	// TODO address tokens longer than a fixed chunk_size of 100
	state->token = malloc(100);

	state->phase = READING_INDENTS;
	state->ln = 1;

	state->indent_controls = malloc(sizeof(int));
	*(state->indent_controls) = CONTROL_WAITING;
	state->indents_tracked = 1;

	return state;
}

// Before parse_token(), we have just read an entire token and can now
// look at it, to place it inside state->cmd
void parse_token(parse_state *state) {
	if (state->phase == READING_NAME && strcmp(state->token, "-") == 0) {
		state->cmd->else_flag = true;
		return;
	}

	printf("On phase %d, adding token |%s|\n", state->phase, state->token);

	// Even if token == command name, set $0 as convention
	add_arg(state->cmd, state->token);

	if (state->phase == READING_NAME) {
		state->cmd->path = get_bin_path(state->token);
		state->phase = READING_ARG;
	}
}

char *control_name(int control) {
	if (control == CONTROL_WAITING)
		return "waiting";

	if (control == CONTROL_BRANCH_ACTIVE)
		return "active";

	if (control == CONTROL_COMPLETE)
		return "complete";

	assert(false);
	return "control not found";
}

void update_control(parse_state *state, int status) {
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
void parse_command(parse_state *state) {
	int indent = state->cmd->indent_level;

	printf("parse_command: P%d, path |%s|\n", state->phase, state->cmd->path);

	// TODO: If a branch shouldn't be executed, all parsing of it should be
	// skipped. Both for performance and because we don't want to evaluate
	// subshells etc. in there
	// 1728777039: We can skip the rest of the line as soon as we read the
	// indent for it and see it's outside of the intended range

	// No command submitted
	// Latter happens if we save "" as the path, through a blank line
	if (state->cmd->path == NULL || *state->cmd->path == '\0') {
		printf("L%d: Blank\n", state->ln);

		// Blank "-\n", equivalent to "- true\n"
		if (state->cmd->else_flag && state->indent_controls[indent] == CONTROL_WAITING)
			update_control(state, CONTROL_BRANCH_ACTIVE);

		state->ln++;
		state->phase = READING_INDENTS;

		clear_command(state->cmd);
		return;
	}

	int parent_control = indent == 0 ? CONTROL_BRANCH_ACTIVE : state->indent_controls[indent-1];
	int control = state->indent_controls[indent];
	printf(">%d:%s, >%d:%s\n", indent-1, control_name(parent_control), indent, control_name(control));

	bool parent_permits = parent_control == CONTROL_BRANCH_ACTIVE;

	// We're not supposed to be executing because the previous branch was active
	if (state->cmd->else_flag && control == CONTROL_BRANCH_ACTIVE)
		update_control(state, CONTROL_COMPLETE);

	if (parent_permits && (!state->cmd->else_flag || control == CONTROL_WAITING)) {
		int exit_code = execute(state->cmd);

		// 0: success exit code, so this if branch is now active
		if (exit_code == 0)
			update_control(state, CONTROL_BRANCH_ACTIVE);

		// The command failed, but this is a new start of a control structure,
		// since it's a base (if). We don't execute this body but we allow
		// further branches to check their conditions
		else if (!state->cmd->else_flag)
			update_control(state, CONTROL_WAITING);

	}

	else printf("L%d: CF skip\n", state->ln);

	state->ln++;
	state->phase = READING_INDENTS;
	clear_command(state->cmd);
}

void parse_script(FILE *script_file) {
	parse_state *state = create_state();

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
		if (state->phase == READING_INDENTS) {
			if (*p == '\t') {
				state->cmd->indent_level++;
				continue;
			}
			else state->phase = READING_NAME;
		}

		// Delete the rest of the line for comments
		if (*p == '#') {
			while (getc(script_file) != '\n') ;
			*p = '\n';
		}

		if (*p != ' ' && *p != '\n') {
			p++;
			continue;
		}

		// We're now ending a token

		char sep = *p;
		*p = '\0';

		parse_token(state);

		if (sep == '\n')
			parse_command(state);

		p = state->token;
	}
}
