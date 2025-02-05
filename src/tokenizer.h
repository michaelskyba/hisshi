#pragma once

#include <stdbool.h> // TokenizerState, read_token

typedef enum TokenType {
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
} TokenType;

struct Token {
	TokenType type;

	// Dynamic char array: text content. Not used for all types
	char *str;

	// Length not including the \0; str's real size is +1
	int str_len;

	// Line number. Stored in the token because it needs to be read while
	// tokenizing, and lets us separate the token from the rest of the state
	int ln;
};

struct Token *create_token();
void free_token(struct Token *tk);

struct TokenizerState {
	// Used to distinguish colons from defining functions or being regular chars
	bool at_line_start;
};

struct TokenizerState *create_tokenizer_state();
char *append_tk_p(struct Token *tk, char *p, char c);
bool read_token(struct Token *tk, struct TokenizerState *state, FILE *fp);
