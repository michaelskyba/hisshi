#ifndef input_source_h_INCLUDED
#define input_source_h_INCLUDED

// Used as a generic so that the tokenizer can read from a file (for a script)
// or from a string (eval, functions, aliases) with a common interface
struct InputSource {
	int (*getc)(struct InputSource *self);
	void (*ungetc)(struct InputSource *self, int c);

	// Internal state specific to the InputSource implementation
	void *state;
};

struct FileInputSourceState {
	FILE *file;

	// Pushback
	char *buf;
	char *buf_p;
};

int file_get_char(struct InputSource *self);
void file_unget_char(struct InputSource *self, int c);
void free_file_input_source(struct InputSource *self);
struct InputSource *create_file_input_source(FILE *file);

struct StringInputSourceState {
	// Stores the main, original string given as input. We shouldn't need a
	// dedicated pushback buffer; pushback is only used for lookahead
	char *buf;
	char *buf_p;
};

int str_get_char(struct InputSource *self);
void str_unget_char(struct InputSource *self, int c);
void free_str_input_source(struct InputSource *source);
struct InputSource *create_str_input_source(char *str);

#endif // input_source_h_INCLUDED
