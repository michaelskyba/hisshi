#ifndef exec_h_INCLUDED
#define exec_h_INCLUDED

struct Command; // command.h
struct ShellState; // shell_state.h

int execute_child(struct Command *cmd, int read_fd, int write_fd, int *pipes, struct ShellState *shell_state);
int get_pipeline_length(struct Command *pipeline);
char *read_pipe_var(int read_fd);
int execute_pipeline(struct Command *pipeline, struct ShellState *shell_state);

#endif // exec_h_INCLUDED
