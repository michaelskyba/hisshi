#pragma once

struct InputSource; // input_source.h
struct ShellState; // shell_state.h
struct Token; // tokenizer.h

void get_function_body_single(struct Token *tk, struct InputSource *source);
void get_function_body_multi(struct Token *tk, int func_indent, struct InputSource *source);

int execute_function(struct ShellState *shell_state, char *func_body_raw);
