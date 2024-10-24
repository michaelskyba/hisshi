// For ParseState.indent_controls
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

// For ParseState.phase
enum {
	READING_INDENTS,
	READING_NAME,
	READING_ARG,
};

typedef struct {
	// The head of a linked list of commands we're constructing
	// "foo | bar | baz" would be three
	Command *cmd_pipeline;

	// Points to the current, which is the last in the list of
	// ParseState.pipeline
	Command *cmd;

	// enum: Which part of the line we're parsing
	int phase;

	Token *tk;

	/*
	A dynamic array storing the control flow status enum at different levels of
	indentation

	indent_controls[n]: status at n levels of indentation
	[0]: base level
	*/
	int *indent_controls;
	int indents_tracked; // total allocated room
} ParseState;

ParseState *create_state() {
	ParseState *state = malloc(sizeof(ParseState));
	state->cmd = create_command();
	state->cmd_pipeline = state->cmd;

	state->phase = READING_INDENTS;

	state->tk = malloc(sizeof(Token));
	state->tk->type = TOKEN_EOF; // Will be replaced when initially read
	state->tk->str = malloc(sizeof(char) * 2); // Include terminator
	state->tk->str_len = 1;
	state->tk->ln = 1;

	state->indent_controls = malloc(sizeof(int));
	*(state->indent_controls) = CONTROL_WAITING;
	state->indents_tracked = 1;

	return state;
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

void update_control(ParseState *state, int status) {
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
void parse_command(ParseState *state) {
	// For now only take the first in the pipeline
	Command *cmd = state->cmd_pipeline;

	int indent = cmd->indent_level;

	// The last token read was the \n, tk->ln is one above cmd
	int ln = state->tk->ln - 1;

	printf("parse_command: P%d, path |%s|\n", state->phase, cmd->path);

	// TODO: If a branch shouldn't be executed, all parsing of it should be
	// skipped. Both for performance and because we don't want to evaluate
	// subshells etc. in there
	// 1728777039: We can skip the rest of the line as soon as we read the
	// indent for it and see it's outside of the intended range
	// 1729642499: Eh maybe we can tokenize it but not evaluate any subshells.
	// Just do whatever is cleaner

	int parent_control = indent == 0 ? CONTROL_BRANCH_ACTIVE : state->indent_controls[indent-1];
	int control = state->indent_controls[indent];
	printf(">%d:%s, >%d:%s\n", indent-1, control_name(parent_control), indent, control_name(control));

	bool parent_permits = parent_control == CONTROL_BRANCH_ACTIVE;

	// We're not supposed to be executing because the previous branch was active
	if (cmd->else_flag && control == CONTROL_BRANCH_ACTIVE)
		update_control(state, CONTROL_COMPLETE);

	if (parent_permits && (!cmd->else_flag || control == CONTROL_WAITING)) {
		// No command submitted
		// Latter happens if we save "" as the path, through a blank line
		if (cmd->path == NULL || *cmd->path == '\0') {
			printf("L%d: Blank\n", ln);

			// Blank "-\n", equivalent to "- true\n"
			if (cmd->else_flag && state->indent_controls[indent] == CONTROL_WAITING)
				update_control(state, CONTROL_BRANCH_ACTIVE);

			return;
		}

		int exit_code = execute(cmd);

		// 0: success exit code, so this if branch is now active
		if (exit_code == 0)
			update_control(state, CONTROL_BRANCH_ACTIVE);

		// The command failed, but this is a new start of a control structure,
		// since it's a base (if). We don't execute this body but we allow
		// further branches to check their conditions
		else if (!cmd->else_flag)
			update_control(state, CONTROL_WAITING);

	}

	else printf("L%d: CF skip\n", ln);
}

void parse_script(FILE *script_file) {
	ParseState *state = create_state();

	while (read_token(state->tk, script_file)) {
		int tk_type = state->tk->type;

		if (tk_type == TOKEN_INDENT) {
			if (state->phase == READING_INDENTS)
				state->cmd->indent_level++;

			// Otherwise, it's just whitespace that we can ignore. We shouldn't
			// panic here, since tab separation, especially after a newline
			// during pipes etc. is a pretty common pattern:
			// echo foo |
			// \ttail |
			// \tcat

			continue;
		}

		// We were reading indents but now finally found a non-indent
		if (state->phase == READING_INDENTS)
			state->phase = READING_NAME;

		if (tk_type == TOKEN_DASH) {
			if (state->phase == READING_NAME)
				state->cmd->else_flag = true;
			else if (state->phase == READING_ARG)
				add_arg(state->cmd, "-");

			continue;
		}

		if (tk_type == TOKEN_PIPE) {
			state->cmd->next_pipeline = create_command();
			state->cmd = state->cmd->next_pipeline;

			state->phase = READING_NAME;
			continue;
		}

		if (tk_type == TOKEN_NAME) {
			printf("Parsing name token |%s|\n", state->tk->str);

			if (state->phase == READING_NAME) {
				state->cmd->path = get_bin_path(state->tk->str);
				state->phase = READING_ARG;
			}

			// Even if READING_NAME, set $0 as convention
			add_arg(state->cmd, state->tk->str);
			continue;
		}

		if (tk_type == TOKEN_NEWLINE) {
			Command *cmd;
			for (cmd = state->cmd_pipeline; cmd != NULL; cmd = cmd->next_pipeline) {
				printf("pipeline: ");
				dump_command(cmd);
			}
			printf("We're only going to the submit the first.\n");

			parse_command(state);
			state->phase = READING_INDENTS;

			clear_command(state->cmd_pipeline);
			state->cmd = state->cmd_pipeline;

			continue;
		}

		// Invalid token type returned if nothing has triggered
		assert(false);
	}
}
