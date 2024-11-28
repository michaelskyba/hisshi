// The command is finished being read, so we examine its context within the
// control flow structure and potentially execute it
void parse_command(ParseState *parse_state, ShellState *shell_state) {
	// Control flow (indent level, else flag) is tracked on the head
	Command *cmd = parse_state->cmd_pipeline;

	int indent = cmd->indent_level;

	// The last token read was the \n, tk->ln is one above cmd
	int ln = parse_state->tk->ln - 1;

	printf("parse_command: P%d, path |%s|\n", parse_state->phase, cmd->path);

	// TODO: If a branch shouldn't be executed, all parsing of it should be
	// skipped. Both for performance and because we don't want to evaluate
	// subshells etc. in there
	// 1728777039: We can skip the rest of the line as soon as we read the
	// indent for it and see it's outside of the intended range
	// 1729642499: Eh maybe we can tokenize it but not evaluate any subshells.
	// Just do whatever is cleaner

	// If the user spammed tabs on a new line with no parent that dynamically
	// resized indent_controls
	assert(parse_state->indents_tracked > indent);

	int parent_control = indent == 0 ? CONTROL_BRANCH_ACTIVE : parse_state->indent_controls[indent-1];
	int control = parse_state->indent_controls[indent];
	printf(">%d:%s, >%d:%s\n", indent-1, control_name(parent_control), indent, control_name(control));

	bool parent_permits = parent_control == CONTROL_BRANCH_ACTIVE;

	// We're not supposed to be executing because the previous branch was active
	if (cmd->else_flag && control == CONTROL_BRANCH_ACTIVE)
		update_control(parse_state, CONTROL_COMPLETE);

	if (parent_permits && (!cmd->else_flag || control == CONTROL_WAITING)) {
		if (cmd->path == NULL) {
			// cmd->path should only be blank if we submitted a blank "-\n". If
			// you submit a completely blank line, parse_command shouldn't be
			// called.
			assert(cmd->else_flag);

			// If so, it's equivalent to "- true\n"
			update_control(parse_state, CONTROL_BRANCH_ACTIVE);
			return;
		}

		int exit_code = execute_pipeline(parse_state->cmd_pipeline, shell_state);

		// 0: success exit code, so this if branch is now active
		if (exit_code == 0)
			update_control(parse_state, CONTROL_BRANCH_ACTIVE);

		// The command failed, but this is a new start of a control structure,
		// since it's a base (if). We don't execute this body but we allow
		// further branches to check their conditions
		else if (!cmd->else_flag)
			update_control(parse_state, CONTROL_WAITING);

	}

	else printf("L%d: CF skip\n", ln);
}

void parse_script(FILE *script_file, ParseState *parse_state, ShellState *shell_state) {
	// Start with at_line_start == true
	TokenizerState *tokenizer_state = create_tokenizer_state();

	while (read_token(parse_state->tk, tokenizer_state, script_file)) {
		int tk_type = parse_state->tk->type;

		if (tk_type == TOKEN_INDENT) {
			if (parse_state->phase == READING_INDENTS)
				parse_state->cmd->indent_level++;

			/*
			Otherwise, it's just whitespace that we can ignore. We shouldn't
			panic here, since tab separation, especially after a newline
			during pipes etc. is a pretty common pattern:
			echo foo |
				tail |
				cat
			*/

			continue;
		}

		// We were reading indents but now finally found a non-indent
		if (parse_state->phase == READING_INDENTS)
			parse_state->phase = READING_NAME;

		if (tk_type == TOKEN_DASH) {
			if (parse_state->phase == READING_NAME)
				parse_state->cmd->else_flag = true;
			else if (parse_state->phase == READING_ARG)
				add_arg(parse_state->cmd, "-");

			continue;
		}

		// We're done with indents, and the current token isn't a dash
		tokenizer_state->at_line_start = false;

		if (
			tk_type == TOKEN_REDIRECT_READ ||
			tk_type == TOKEN_REDIRECT_WRITE ||
			tk_type == TOKEN_REDIRECT_APPEND
		) {
			int redirect_type = tk_type;

			// TODO variable etc. support
			read_token(parse_state->tk, tokenizer_state, script_file);
			assert(parse_state->tk->type == TOKEN_NAME);
			char *filename = get_str_copy(parse_state->tk->str);

			printf("Read redirection (%d) filename |%s|\n", redirect_type, filename);

			if (redirect_type == TOKEN_REDIRECT_READ)
				parse_state->cmd->redirect_read = filename;
			if (redirect_type == TOKEN_REDIRECT_WRITE)
				parse_state->cmd->redirect_write = filename;
			if (redirect_type == TOKEN_REDIRECT_APPEND)
				parse_state->cmd->redirect_append = filename;

			continue;
		}

		if (tk_type == TOKEN_PIPE_VARIABLE) {
			// TODO variable (date |= $foo) etc. support
			read_token(parse_state->tk, tokenizer_state, script_file);
			assert(parse_state->tk->type == TOKEN_NAME);

			char *var_name = get_str_copy(parse_state->tk->str);
			parse_state->cmd->pipe_variable = var_name;

			// Don't worry about cmd because we assume this will be the last
			// token before TOKEN_NEWLINE

			printf("Ack intention to pipe to var |%s|\n", var_name);
			continue;
		}

		if (tk_type == TOKEN_PIPE) {
			parse_state->cmd->next_pipeline = create_command();
			parse_state->cmd = parse_state->cmd->next_pipeline;

			parse_state->phase = READING_NAME;
			continue;
		}

		if (tk_type == TOKEN_FUNC_NAME_SINGLE) {
			// Needs to be copied manually or else overwritten by
			// get_function_body
			char *func_name = get_str_copy(parse_state->tk->str);

			get_function_body_single(parse_state->tk, script_file);
			char *func_body = parse_state->tk->str;

			set_function(shell_state, func_name, func_body);
			free(func_name);
			continue;
		}

		if (tk_type == TOKEN_FUNC_NAME_MULTI) {
			char *func_name = get_str_copy(parse_state->tk->str);

			int body_indent_level = parse_state->cmd->indent_level + 1;
			get_function_body_multi(parse_state->tk, body_indent_level, script_file);
			char *func_body = parse_state->tk->str;

			set_function(shell_state, func_name, func_body);
			free(func_name);
			continue;
		}

		if (tk_type == TOKEN_NAME || tk_type == TOKEN_VARIABLE) {
			char *str = parse_state->tk->str;

			if (tk_type == TOKEN_VARIABLE) {
				printf("Script asks for var |%s|\n", str);
				str = get_variable(shell_state, str);

				if (str == NULL)
					str = "";

				printf("We queried and received value |%s|\n", str);
			}

			printf("Parsing str token |%s|\n", str);

			if (parse_state->phase == READING_NAME) {
				parse_state->cmd->path = get_bin_path(str);
				parse_state->phase = READING_ARG;
			}

			// Even if READING_NAME, set $0 as convention
			add_arg(parse_state->cmd, str);
			continue;
		}

		if (tk_type == TOKEN_NEWLINE) {
			tokenizer_state->at_line_start = true;

			// User is splitting the next command across multiple lines, or is
			// otherwise leaving it blank (supported)
			if (parse_state->phase == READING_NAME && !parse_state->cmd->else_flag) {
				// If this is an entirely blank submission, then the next line
				// still needs to read indents again
				if (parse_state->cmd_pipeline->path == NULL)
					parse_state->phase = READING_INDENTS;

				continue;
			}

			parse_command(parse_state, shell_state);

			clear_command(parse_state->cmd_pipeline);
			parse_state->cmd = parse_state->cmd_pipeline;
			parse_state->phase = READING_INDENTS;

			continue;
		}

		// Invalid token type returned if nothing has triggered
		printf("Received invalid token type %d\n", tk_type);
		assert(false);
	}

	free(tokenizer_state);
}
