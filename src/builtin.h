#ifndef builtin_h_INCLUDED
#define builtin_h_INCLUDED

typedef struct Command Command; // command.h
typedef struct ShellState ShellState; // shell_state.h

int builtin_cd(Command *cmd, ShellState *state);
int builtin_exit(Command *cmd, ShellState *state);
int builtin_export(Command *cmd, ShellState *state);
int builtin_unset(Command *cmd, ShellState *state);

int (*get_builtin(char *name)) (Command *, ShellState *);

#endif // builtin_h_INCLUDED
