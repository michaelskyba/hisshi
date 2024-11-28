// For Token.type
enum {
	TOKEN_INDENT,
	TOKEN_DASH,
	TOKEN_NAME,
	TOKEN_FUNC_NAME,
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
	while ((c = getc(script_file)) == ' ') ;

	if (c == EOF) {
		tk->type = TOKEN_EOF;
		return false;
	}

	if (c == '<') {
		tk->type = TOKEN_REDIRECT_READ;
		return true;
	}

	if (c == '>') {
		c = getc(script_file);

		if (c == '>')
			tk->type = TOKEN_REDIRECT_APPEND;
		else {
			tk->type = TOKEN_REDIRECT_WRITE;
			ungetc(c, script_file);
		}

		return true;
	}

	if (c == '\t') {
		tk->type = TOKEN_INDENT;
		return true;
	}

	if (c == '#') {
		while (getc(script_file) != '\n') ;
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
		c = getc(script_file);

		if (c == '=')
			tk->type = TOKEN_PIPE_VARIABLE;
		else {
			tk->type = TOKEN_PIPE;
			ungetc(c, script_file);
		}

		return true;
	}

	if (c == '-') {
		c = getc(script_file);
		ungetc(c, script_file);

		if (isspace(c) || c == '#') {
			tk->type = TOKEN_DASH;
			return true;
		}

		// Otherwise it's part of a name
		c = '-';
	}

	if (c == '$') {
		c = getc(script_file);
		assert(isalnum(c));
		ungetc(c, script_file);

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

		while ((c = getc(script_file)) != match) {
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

			c = getc(script_file);
		}

		// Defined as ^myfunc:
		if (state->at_line_start && *(p-1) == ':') {
			tk->type = TOKEN_FUNC_NAME;
			printf("Func decl ending: %d\n", c);

			// Don't include the colon in tk->str
			*(p-1) = '\0';
		}
		else {
			tk->type = TOKEN_NAME;
			*p = '\0';
		}

		ungetc(c, script_file);
		return true;
	}
}
