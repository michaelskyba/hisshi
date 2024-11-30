#ifndef parser_h_INCLUDED
#define parser_h_INCLUDED

typedef struct ParseState ParseState; // parse_state.h
typedef struct ShellState ShellState; // shell_state.h
typedef struct InputSource InputSource; // input_source.h

void parse_command(ParseState *parse_state, ShellState *shell_state);
void parse_script(ParseState *parse_state, ShellState *shell_state, InputSource *source);

#endif // parser_h_INCLUDED
