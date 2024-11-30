#ifndef input_source_h_INCLUDED
#define input_source_h_INCLUDED

// Used as a generic so that the tokenizer can read from a file (for a script)
// or from a string (eval, functions, aliases) with a common interface
typedef struct InputSource {
	int (*getc)(struct InputSource *self);
	void (*ungetc)(struct InputSource *self, int c);

	// Internal state specific to the InputSource implementation
	void *state;
} InputSource;

typedef struct FileInputSourceState {
	FILE *file;

	// Pushback
	char *buf;
	char *buf_p;
} FileInputSourceState;

int file_get_char(InputSource *self);
void file_unget_char(InputSource *self, int c);
void free_file_input_source(InputSource *self);
InputSource *create_file_input_source(FILE *file);

typedef struct StringInputSourceState {
	// Stores the main, original string given as input. We shouldn't need a
	// dedicated pushback buffer; pushback is only used for lookahead
	char *buf;
	char *buf_p;
} StringInputSourceState;

int str_get_char(InputSource *self);
void str_unget_char(InputSource *self, int c);
void free_str_input_source(InputSource *source);
InputSource *create_str_input_source(char *str);

#endif // input_source_h_INCLUDED
