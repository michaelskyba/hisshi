#ifndef exec_h_INCLUDED
#define exec_h_INCLUDED

typedef struct Command Command; // command.h
typedef struct ShellState ShellState; // shell_state.h

int execute_child(Command *cmd, int read_fd, int write_fd, int *pipes, ShellState *shell_state);
int get_pipeline_length(Command *pipeline);
char *read_pipe_var(int read_fd);
int execute_pipeline(Command *pipeline, ShellState *shell_state);

#endif // exec_h_INCLUDED
