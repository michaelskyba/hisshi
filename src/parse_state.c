#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "parse_state.h"
#include "tokenizer.h"
#include "command.h"

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

		printf("reallocating indent tracker to size %d\n", state->indents_tracked);
	}

	/*
	If we still have other indents tracked beyond this one, reset them to
	prevent future overlap. e.g. We don't want cmd to run in:
	true
		true
	-
		false
			command
	but it otherwise would, because the second true sets the second indent to
	active, and false doesn't run so it doesn't override
	*/
	for (int i = indent+1; i < state->indents_tracked; i++)
		state->indent_controls[i] = CONTROL_WAITING;
}
