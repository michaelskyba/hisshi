#pragma once

typedef enum ControlStatus {
	// Default: no branch of this control flow structure has matched. elifs and
	// elses can still activate.
	CONTROL_WAITING,

	// The current branch that we're parsing commands within was matched and is
	// active. The body should execute.
	CONTROL_BRANCH_ACTIVE,

	// The current branch is inactive, since we've matched a previous branch
	// within the same control structure. Any new bodies should not execute.
	CONTROL_COMPLETE,
} ControlStatus;

typedef enum ParsePhase {
	READING_INDENTS,
	READING_NAME,
	READING_ARG,
} ParsePhase;

struct ParseState {
	// The head of a linked list of commands we're constructing
	// "foo | bar | baz" would be three
	struct Command *cmd_pipeline;

	// Points to the current, which is the last in the list of
	// ParseState.pipeline
	struct Command *cmd;

	// Which part of the line we're parsing
	ParsePhase phase;

	// TODO For now this would probably be better to store externally, created
	// and freed in parse_script, but we probably want to track the line number
	struct Token *tk;

	/*
	A dynamic array storing the control flow status enum at different levels of
	indentation

	indent_controls[n]: status at n levels of indentation
	[0]: base level
	*/
	ControlStatus *indent_controls;
	int indents_tracked; // total allocated room. starts at 1
};

struct ParseState *create_parse_state();
void free_parse_state(struct ParseState *state);
char *control_name(int control);
void update_control(struct ParseState *state, int status);
