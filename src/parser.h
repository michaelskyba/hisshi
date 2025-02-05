#pragma once

struct Command; // command.h
struct ParseState; // parse_state.h
struct ShellState; // shell_state.h

void parse_command(struct ParseState *parse_state, struct ShellState *shell_state);
void parse_script(struct ParseState *parse_state, struct ShellState *shell_state, FILE *fp);

int eval_function(struct Command *cmd, struct ShellState *parent, char *func_body_raw);
