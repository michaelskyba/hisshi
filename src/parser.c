// The command is finished being read, so we examine its context within the
// control flow structure and potentially execute it
void parse_command(ParseState *state) {
	// Control flow (indent level, else flag) is tracked on the head
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
		if (cmd->path == NULL) {
			// cmd->path should only be blank if we submitted a blank "-\n". If
			// you submit a completely blank line, parse_command shouldn't be
			// called.
			assert(cmd->else_flag);

			// If so, it's equivalent to "- true\n"
			update_control(state, CONTROL_BRANCH_ACTIVE);
			return;
		}

		int exit_code = execute_pipeline(state->cmd_pipeline);

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

		if (tk_type == TOKEN_NAME || tk_type == TOKEN_VARIABLE) {
			if (tk_type == TOKEN_NAME)
				printf("Parsing name token |%s|\n", state->tk->str);
			else
				printf("Parsing variable token. Temp taking value as |%s|\n", state->tk->str);

			if (state->phase == READING_NAME) {
				state->cmd->path = get_bin_path(state->tk->str);
				state->phase = READING_ARG;
			}

			// Even if READING_NAME, set $0 as convention
			add_arg(state->cmd, state->tk->str);
			continue;
		}

		if (tk_type == TOKEN_NEWLINE) {
			// They had the bright idea of splitting the next command across
			// multiple lines, or otherwise leaving it blank
			// (Acceptable)
			if (state->phase == READING_NAME && !state->cmd->else_flag)
				continue;

			parse_command(state);

			clear_command(state->cmd_pipeline);
			state->cmd = state->cmd_pipeline;
			state->phase = READING_INDENTS;

			continue;
		}

		// Invalid token type returned if nothing has triggered
		assert(false);
	}
}
