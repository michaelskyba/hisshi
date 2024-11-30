#ifndef parser_h_INCLUDED
#define parser_h_INCLUDED

struct ParseState; // parse_state.h
struct ShellState; // shell_state.h
struct InputSource; // input_source.h

void parse_command(struct ParseState *parse_state, struct ShellState *shell_state);
void parse_script(struct ParseState *parse_state, struct ShellState *shell_state, struct InputSource *source);

#endif // parser_h_INCLUDED
