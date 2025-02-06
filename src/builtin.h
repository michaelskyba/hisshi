#pragma once

struct Command; // command.h
struct ShellState; // shell_state.h

int builtin_cd(struct Command *cmd, struct ShellState *state);
int builtin_exit(struct Command *cmd, struct ShellState *state);
int builtin_export(struct Command *cmd, struct ShellState *state);
int builtin_unset(struct Command *cmd, struct ShellState *state);
int builtin_eval(struct Command *cmd, struct ShellState *state);
int builtin_global(struct Command *cmd, struct ShellState *state);

int (*get_builtin(char *name)) (struct Command *, struct ShellState *);
