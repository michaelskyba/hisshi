// Looking ahead is generally only required in misc one-char situations, or for
// looking at leading indents to check for the end of a function body
#define INPUT_PUSHBACK_BUFSIZ 32

// Used as a generic so that the tokenizer can read from a file (for a script)
// or from a string (eval, functions, aliases) with a common interface
typedef struct InputSource {
	int (*getc)(struct InputSource *self);
	void (*ungetc)(struct InputSource *self, int c);

	// Internal state specific to the InputSource implementation
	void *state;
} InputSource;

typedef struct {
	FILE *file;

	// Pushback
	char *buf;
	char *buf_p;
} FileInputSourceState;

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
