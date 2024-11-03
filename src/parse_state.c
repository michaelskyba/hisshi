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
	int indents_tracked; // total allocated room. starts at 1
} ParseState;

ParseState *create_parse_state() {
	ParseState *state = malloc(sizeof(ParseState));
	state->cmd = create_command();
	state->cmd_pipeline = state->cmd;

	state->phase = READING_INDENTS;
	state->tk = create_token();

	state->indent_controls = malloc(sizeof(int));
	*(state->indent_controls) = CONTROL_WAITING;
	state->indents_tracked = 1;

	return state;
}

void free_parse_state(ParseState *state) {
	free_command(state->cmd_pipeline);
	free_token(state->tk);
	free(state->indent_controls);
	free(state);
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
	int indent = state->cmd_pipeline->indent_level;
	state->indent_controls[indent] = status;

	printf("Received >%d:%s\n", indent, control_name(status));

	if (indent+1 == state->indents_tracked) {
		state->indents_tracked *= 2;

		int size = sizeof(int) * state->indents_tracked;
		state->indent_controls = realloc(state->indent_controls, size);

		// Initialize default value for new batch
		int original_tracked = state->indents_tracked/2;
		for (int i = original_tracked; i < state->indents_tracked; i++)
			state->indent_controls[i] = CONTROL_WAITING;

		printf("reallocating indent tracker to size %d\n", state->indents_tracked);
	}
}
