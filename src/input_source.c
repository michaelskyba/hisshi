#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "input_source.h"

typedef struct FileInputSourceState FileInputSourceState;
typedef struct InputSource InputSource;
typedef struct StringInputSourceState StringInputSourceState;

// Looking ahead is generally only required in misc one-char situations, or for
// looking at leading indents to check for the end of a function body
#define INPUT_PUSHBACK_BUFSIZ 32

int file_get_char(InputSource *self) {
	FileInputSourceState *state = (FileInputSourceState *) self->state;

	if (state->buf_p > state->buf)
		return *(--state->buf_p);

	return fgetc(state->file);
}

void file_unget_char(InputSource *self, int c) {
	FileInputSourceState *state = (FileInputSourceState *) self->state;
	*(state->buf_p++) = c;
}

// Function to destroy the FileInputSource.
void free_file_input_source(InputSource *self) {
	FileInputSourceState *state = (FileInputSourceState *) self->state;

	if (state->file)
		fclose(state->file);

	free(state->buf);
	free(state);
	free(self);
}

InputSource *create_file_input_source(FILE *file) {
	InputSource *source = malloc(sizeof(InputSource));
	source->getc = file_get_char;
	source->ungetc = file_unget_char;

	FileInputSourceState *state = malloc(sizeof(FileInputSourceState));
	state->buf = malloc(INPUT_PUSHBACK_BUFSIZ);
	state->buf_p = state->buf;
	state->file = file;

	source->state = state;
	return source;
}

int str_get_char(InputSource *self) {
	StringInputSourceState *state = (StringInputSourceState *) self->state;
	return *(state->buf_p++);
}

void str_unget_char(InputSource *self, int c) {
	StringInputSourceState *state = (StringInputSourceState *)self->state;

	// Not really necessary to take any char at all because our input is
	// supposed to be determined at InputSource create-time, but let's stay
	// consistent
	assert(*(--state->buf_p) == c);
}

void free_str_input_source(InputSource *source) {
	StringInputSourceState *state = (StringInputSourceState *)source->state;

	free(state->buf);
	free(state);
	free(source);
}

InputSource *create_str_input_source(char *str) {
	InputSource *source = malloc(sizeof(InputSource));
	source->getc = str_get_char;
	source->ungetc = str_unget_char;

	StringInputSourceState *state = malloc(sizeof(StringInputSourceState));
	state->buf = str;
	state->buf_p = state->buf;

	source->state = state;
	return source;
}
