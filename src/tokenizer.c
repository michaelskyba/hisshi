#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "tokenizer.h"
#include "util.h"

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

		debug("Reallocating tk->str to size %d\n", tk->str_len);

		// str_len doesn't include \0
		tk->str = realloc(tk->str, tk->str_len + 1);
		p = tk->str + offset;
	}

	return p;
}

// tk->str will be overwritten, so copy it if you need it
// rt: whether to keep reading (type != EOF)
bool read_token(Token *tk, TokenizerState *state, FILE *fp) {
	char c;
	while ((c = fgetc(fp)) == ' ') ;

	if (c == EOF) {
		tk->type = TOKEN_EOF;
		return false;
	}

	if (c == '<') {
		tk->type = TOKEN_REDIRECT_READ;
		return true;
	}

	if (c == '>') {
		c = fgetc(fp);

		if (c == '>')
			tk->type = TOKEN_REDIRECT_APPEND;
		else {
			tk->type = TOKEN_REDIRECT_WRITE;
			ungetc(c, fp);
		}

		return true;
	}

	if (c == '\t') {
		tk->type = TOKEN_INDENT;
		return true;
	}

	if (c == '#') {
		while (fgetc(fp) != '\n') ;
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
		c = fgetc(fp);

		if (c == '=')
			tk->type = TOKEN_PIPE_VARIABLE;
		else {
			tk->type = TOKEN_PIPE;
			ungetc(c, fp);
		}

		return true;
	}

	if (c == '-') {
		char peek = fgetc(fp);
		ungetc(peek, fp);

		if (isspace(peek) || peek == '#') {
			tk->type = TOKEN_DASH;
			return true;
		}

		// Otherwise it's part of a name
		c = '-';
	}

	if (c == '$') {
		c = fgetc(fp);
		assert(isalnum(c) || c == '_' || c == '?' || c == '@');
		ungetc(c, fp);

		// For now rely on regular name parsing for the variable name,
		// but overwrite the type

		if (!read_token(tk, state, fp))
			return false;

		tk->type = TOKEN_VARIABLE;
		return true;
	}

	if (c == '"' || c == '\'') {
		char match = c;
		tk->type = TOKEN_NAME;

		// Don't insert the initial "/'
		char *p = tk->str;

		int ln_start = tk->ln;

		while ((c = fgetc(fp)) != match) {
			if (c == EOF) {
				fprintf(stderr, "[%d-%d]: unclosed string\n", ln_start, tk->ln);
				exit(1);
			}

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
			c = fgetc(fp);
		}

		// Defined as ^\t*myfunc:
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

		ungetc(c, fp);
		return true;
	}
}
