// For Token.type
enum {
	TOKEN_INDENT,
	TOKEN_DASH,
	TOKEN_NAME,
	TOKEN_VARIABLE,
	TOKEN_PIPE,
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

// rt: whether to keep reading (type != EOF)
bool read_token(Token *tk, FILE *script_file) {
	char c;
	while ((c = getc(script_file)) == ' ') ;

	if (c == EOF) {
		tk->type = TOKEN_EOF;
		return false;
	}

	if (c == '\t') {
		tk->type = TOKEN_INDENT;
		return true;
	}

	if (c == '#') {
		while (getc(script_file) != '\n') ;
		tk->type = TOKEN_NEWLINE;
		return true;
	}

	if (c == '\n') {
		tk->type = TOKEN_NEWLINE;
		tk->ln++;

		return true;
	}

	if (c == '|') {
		tk->type = TOKEN_PIPE;
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
	}

	if (c == '$') {
		c = getc(script_file);
		assert(isalnum(c));
		ungetc(c, script_file);

		// For now rely on regular name parsing for the variable name,
		// but overwrite the type

		if (!read_token(tk, script_file))
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

			if (p - tk->str == tk->str_len) {
				int offset = p - tk->str;
				tk->str_len *= 2;

				// str_len doesn't include \0
				tk->str = realloc(tk->str, tk->str_len + 1);
				p = tk->str + offset;
			}
		}

		*p = '\0';
		return true;
	}

	tk->type = TOKEN_NAME;

	char *p = tk->str;
	while (!isspace(c) && c != '#') {
		assert(c != EOF);
		*p++ = c;

		if (p - tk->str == tk->str_len) {
			int offset = p - tk->str;
			tk->str_len *= 2;

			// str_len doesn't include \0
			tk->str = realloc(tk->str, tk->str_len + 1);
			p = tk->str + offset;
		}

		c = getc(script_file);
	}

	ungetc(c, script_file);

	*p = '\0';
	return true;
}
