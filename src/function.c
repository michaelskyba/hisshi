#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#include "function.h"
#include "input_source.h"
#include "parse_state.h"
#include "parser.h"
#include "shell_state.h"
#include "tokenizer.h"
#include "util.h"

typedef struct InputSource InputSource;
typedef struct ParseState ParseState;
typedef struct ShellState ShellState;
typedef struct Token Token;

// Reads until the end of the line into the given token. Used to get the body of
// a single-line functions.
void get_function_body_single(Token *tk, InputSource *source) {
	char *p = tk->str;
	char c = source->getc(source);

	// It wouldn't make a functional difference but remove leading spaces
	// Usually we write functions like "myfunc: echo foo" and not "myfunc:echo foo"
	while (c != '\n' && isspace(c))
		c = source->getc(source);

	// c may include comments too, but we can just store them in the function
	// body and ignore them when evaling later
	while (c != '\n') {
		assert(c != EOF);

		p = append_tk_p(tk, p, c);
		c = source->getc(source);
	}

	p = append_tk_p(tk, p, '\n');
	*p = '\0';

	source->ungetc(source, c);
}

/*
func_indent: The base indent level of the function's body, which is one more
than the indent level that the name was defined at
--
This reads a multi-line function body into the given token. All content will be
preserved except that indentation will now happen relative to indent_level.

e.g.
true
	myfunc:
		date
		false
			echo bar
	pwd

This will be returned with 0 tabs before date or false, and 1 tab before echo.
It will exit consuming the final echo newline but not consuming pwd's indent.
*/
void get_function_body_multi(Token *tk, int func_indent, InputSource *source) {
	char *p = tk->str;

	// The newline after the function name
	assert(source->getc(source) == '\n');

	while (true) {
		char c = source->getc(source);

		// Lookahead for indent of the current line
		int line_indent = 0;
		while (c == '\t') {
			line_indent++;
			c = source->getc(source);
		}

		printf("Inside function body, read >%d, c=%c\n", line_indent, c);

		// If the line is blank, we consider it part of the function either way
		if (c == '\n') {
			for (int i = 0; i < line_indent - func_indent; i++)
				p = append_tk_p(tk, p, '\t');

			p = append_tk_p(tk, p, '\n');
			tk->ln++;

			continue;
		}

		// If the function body is finished, give back the lookahead and exit
		if (line_indent < func_indent) {
			source->ungetc(source, c);
			for (int i = 0; i < line_indent; i++)
				source->ungetc(source, '\t');

			*p = '\0';
			return;
		}

		// Add the relative indent of the current line to the token buffer
		for (int i = 0; i < line_indent - func_indent; i++)
			p = append_tk_p(tk, p, '\t');

		// Add the rest of this line to the token buffer
		while (c != '\n') {
			assert(c != EOF);

			p = append_tk_p(tk, p, c);
			c = source->getc(source);
		}

		p = append_tk_p(tk, p, '\n');
		tk->ln++;
	}
}

// Returns exit code
// Makes a copy of func_body
int execute_function(ShellState *parent, char *func_body_raw) {
	// We include the \n in the body when tokenizing, so we don't need to add a
	// trailing one here
	char *func_body = get_str_copy(func_body_raw);
	InputSource *source = create_str_input_source(func_body);

	ParseState *parse_state = create_parse_state();
	ShellState *shell_state = create_shell_state(parent);

	parse_script(parse_state, shell_state, source);

	int exit_code = shell_state->exit_code;

	free_parse_state(parse_state);
	free_shell_state(shell_state);
	free_str_input_source(source);

	return exit_code;
}
