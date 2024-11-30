#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "input_source.h"
#include "tokenizer.h"

typedef struct InputSource InputSource;
typedef struct TokenizerState TokenizerState;
typedef struct Token Token;

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

TokenizerState *create_tokenizer_state() {
	TokenizerState *state = malloc(sizeof(TokenizerState));
	state->at_line_start = true;
	return state;
}

// Returns new value of p
char *append_tk_p(Token *tk, char *p, char c) {
	*p++ = c;

	if (p - tk->str == tk->str_len) {
		int offset = p - tk->str;
		tk->str_len *= 2;

		printf("Reallocating tk->str to size %d\n", tk->str_len);

		// str_len doesn't include \0
		tk->str = realloc(tk->str, tk->str_len + 1);
		p = tk->str + offset;
	}

	return p;
}

// tk->str will be overwritten, so copy it if you need it
// rt: whether to keep reading (type != EOF)
bool read_token(Token *tk, TokenizerState *state, InputSource *source) {
	char c;
	while ((c = source->getc(source)) == ' ') ;

	if (c == EOF) {
		tk->type = TOKEN_EOF;
		return false;
	}

	if (c == '<') {
		tk->type = TOKEN_REDIRECT_READ;
		return true;
	}

	if (c == '>') {
		c = source->getc(source);

		if (c == '>')
			tk->type = TOKEN_REDIRECT_APPEND;
		else {
			tk->type = TOKEN_REDIRECT_WRITE;
			source->ungetc(source, c);
		}

		return true;
	}

	if (c == '\t') {
		tk->type = TOKEN_INDENT;
		return true;
	}

	if (c == '#') {
		while (source->getc(source) != '\n') ;
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
		c = source->getc(source);

		if (c == '=')
			tk->type = TOKEN_PIPE_VARIABLE;
		else {
			tk->type = TOKEN_PIPE;
			source->ungetc(source, c);
		}

		return true;
	}

	if (c == '-') {
		char peek = source->getc(source);
		source->ungetc(source, peek);

		if (isspace(peek) || peek == '#') {
			tk->type = TOKEN_DASH;
			return true;
		}

		// Otherwise it's part of a name
		c = '-';
	}

	if (c == '$') {
		c = source->getc(source);
		assert(isalnum(c));
		source->ungetc(source, c);

		// For now rely on regular name parsing for the variable name,
		// but overwrite the type

		if (!read_token(tk, state, source))
			return false;

		tk->type = TOKEN_VARIABLE;
		return true;
	}

	if (c == '"' || c == '\'') {
		char match = c;
		tk->type = TOKEN_NAME;

		// Don't insert the initial "/'
		char *p = tk->str;

		while ((c = source->getc(source)) != match) {
			assert(c != EOF);

			if (c == '\n')
				tk->ln++;

			p = append_tk_p(tk, p, c);
		}

		*p = '\0';
		return true;
	}

	// Unquoted regular or function name
	else {
		char *p = tk->str;

		while (!isspace(c) && c != '#') {
			assert(c != EOF);

			p = append_tk_p(tk, p, c);
			c = source->getc(source);
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

		source->ungetc(source, c);
		return true;
	}
}
