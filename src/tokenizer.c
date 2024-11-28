// For Token.type
enum {
	TOKEN_INDENT,
	TOKEN_DASH,
	TOKEN_NAME,
	TOKEN_FUNC_NAME_SINGLE, // body defined on the same line as the name
	TOKEN_FUNC_NAME_MULTI, // body defined across indented lines
	TOKEN_VARIABLE,
	TOKEN_REDIRECT_READ,
	TOKEN_PIPE,
	TOKEN_PIPE_VARIABLE,
	TOKEN_REDIRECT_WRITE,
	TOKEN_REDIRECT_APPEND,
	TOKEN_NEWLINE,
	TOKEN_EOF,
};

typedef struct {
	int type;

	// Dynamic char array: text content. Not used for all types
	char *str;

	// Length not including the \0; str's real size is +1
	int str_len;

	// Line number. Stored in the token because it needs to be read while
	// tokenizing, and lets us separate the token from the rest of the state
	int ln;
} Token;

Token *create_token() {
	Token *tk = malloc(sizeof(Token));
	tk->type = TOKEN_EOF; // Will be replaced when initially read
	tk->str = malloc(2); // Include terminator
	tk->str_len = 1;
	tk->ln = 1;
	return tk;
}

void free_token(Token *tk) {
	free(tk->str);
	free(tk);
}

typedef struct {
	// Used to distinguish colons from defining functions or being regular chars
	bool at_line_start;
} TokenizerState;

TokenizerState *create_tokenizer_state() {
	TokenizerState *state = malloc(sizeof(TokenizerState));
	state->at_line_start = true;
	return state;
}

// Returns the same p but relative to the new str pointer
char *resize_tk_str(Token *tk, char *p) {
	int offset = p - tk->str;
	tk->str_len *= 2;

	// str_len doesn't include \0
	tk->str = realloc(tk->str, tk->str_len + 1);
	return tk->str + offset;
}

// rt: whether to keep reading (type != EOF)
// tk->str will be overwritten, so copy it if you need it
bool read_token(Token *tk, TokenizerState *state, FILE *script_file) {
	char c;
	while ((c = getcb(script_file)) == ' ') ;

	if (c == EOF) {
		tk->type = TOKEN_EOF;
		return false;
	}

	if (c == '<') {
		tk->type = TOKEN_REDIRECT_READ;
		return true;
	}

	if (c == '>') {
		c = getcb(script_file);

		if (c == '>')
			tk->type = TOKEN_REDIRECT_APPEND;
		else {
			tk->type = TOKEN_REDIRECT_WRITE;
			ungetcb(c);
		}

		return true;
	}

	if (c == '\t') {
		tk->type = TOKEN_INDENT;
		return true;
	}

	if (c == '#') {
		while (getcb(script_file) != '\n') ;
		tk->type = TOKEN_NEWLINE;
		tk->ln++;

		return true;
	}

	if (c == '\n') {
		tk->type = TOKEN_NEWLINE;
		tk->ln++;

		return true;
	}

	if (c == '|') {
		c = getcb(script_file);

		if (c == '=')
			tk->type = TOKEN_PIPE_VARIABLE;
		else {
			tk->type = TOKEN_PIPE;
			ungetcb(c);
		}

		return true;
	}

	if (c == '-') {
		c = getcb(script_file);
		ungetcb(c);

		if (isspace(c) || c == '#') {
			tk->type = TOKEN_DASH;
			return true;
		}

		// Otherwise it's part of a name
		c = '-';
	}

	if (c == '$') {
		c = getcb(script_file);
		assert(isalnum(c));
		ungetcb(c);

		// For now rely on regular name parsing for the variable name,
		// but overwrite the type

		if (!read_token(tk, state, script_file))
			return false;

		tk->type = TOKEN_VARIABLE;
		return true;
	}

	if (c == '"' || c == '\'') {
		char match = c;
		tk->type = TOKEN_NAME;

		// Don't insert the initial "/'
		char *p = tk->str;

		while ((c = getcb(script_file)) != match) {
			assert(c != EOF);
			*p++ = c;

			if (c == '\n')
				tk->ln++;

			if (p - tk->str == tk->str_len)
				p = resize_tk_str(tk, p);
		}

		*p = '\0';
		return true;
	}

	// Unquoted regular or function name
	else {
		char *p = tk->str;
		while (!isspace(c) && c != '#') {
			assert(c != EOF);
			*p++ = c;

			if (p - tk->str == tk->str_len)
				p = resize_tk_str(tk, p);

			c = getcb(script_file);
		}

		// Defined as ^myfunc:
		if (state->at_line_start && *(p-1) == ':') {
			char delim = c;
			tk->type = delim == '\n' ? TOKEN_FUNC_NAME_MULTI : TOKEN_FUNC_NAME_SINGLE;

			// Don't include the colon in tk->str
			*(p-1) = '\0';
		}
		else {
			tk->type = TOKEN_NAME;
			*p = '\0';
		}

		ungetcb(c);
		return true;
	}
}

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
		*p++ = c;

		if (p - tk->str == tk->str_len)
			p = resize_tk_str(tk, p);

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

		// If the function body is finished, give back the lookahead and exit
		if (line_indent < func_indent) {

			ungetcb(c);
			for (int i = 0; i < line_indent; i++)
				ungetcb('\t');

			*p = '\0';
			return;
		}

		// Add the relative indent of the current line to the token buffer
		for (int i = 0; i < line_indent - func_indent; i++) {
			*p++ = '\t';

			if (p - tk->str == tk->str_len)
				p = resize_tk_str(tk, p);
		}

		// Add the rest of this line to the token buffer
		while (true) {
			assert(c != EOF);
			*p++ = c;

			if (p - tk->str == tk->str_len)
				p = resize_tk_str(tk, p);

			// if \n, still add it to the buffer before breaking
			if (c == '\n')
				break;

			c = getcb(script_file);
		}

		tk->ln++;
	}
}
