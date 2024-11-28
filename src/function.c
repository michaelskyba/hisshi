// Reads until the end of the line into the given token. Used to get the body of
// a single-line functions.
void get_function_body_single(Token *tk, FILE *script_file) {
	char *p = tk->str;
	char c = getcb(script_file);

	// It wouldn't make a functional difference but remove leading spaces
	// Usually we write functions like "myfunc: echo foo" and not "myfunc:echo foo"
	while (c != '\n' && isspace(c))
		c = getcb(script_file);

	// c may include comments too, but we can just store them in the function
	// body and ignore them when evaling later
	while (c != '\n') {
		assert(c != EOF);

		p = append_tk_p(tk, p, c);
		c = getcb(script_file);
	}

	*p = '\0';
	ungetcb(c);
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
void get_function_body_multi(Token *tk, int func_indent, FILE *script_file) {
	char *p = tk->str;

	// The newline after the function name
	assert(getcb(script_file) == '\n');

	while (true) {
		char c = getcb(script_file);

		// Lookahead for indent of the current line
		int line_indent = 0;
		while (c == '\t') {
			line_indent++;
			c = getcb(script_file);
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

			ungetcb(c);
			for (int i = 0; i < line_indent; i++)
				ungetcb('\t');

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
			c = getcb(script_file);
		}

		p = append_tk_p(tk, p, '\n');
		tk->ln++;
	}
}
