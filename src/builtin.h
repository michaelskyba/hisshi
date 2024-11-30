#ifndef builtin_h_INCLUDED
#define builtin_h_INCLUDED

struct Command; // command.h
struct ShellState; // shell_state.h

int builtin_cd(struct Command *cmd, struct ShellState *state);
int builtin_exit(struct Command *cmd, struct ShellState *state);
int builtin_export(struct Command *cmd, struct ShellState *state);
int builtin_unset(struct Command *cmd, struct ShellState *state);

int (*get_builtin(char *name)) (struct Command *, struct ShellState *);

#endif // builtin_h_INCLUDED
