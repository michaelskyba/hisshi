#ifndef command_h_INCLUDED
#define command_h_INCLUDED

typedef struct ArgNode {
	char *name;
	struct ArgNode *next;
} ArgNode;

typedef struct Command {
	char *path;

	// The arg list and argc include the $0 name
	int argc;
	ArgNode *arg_head;
	ArgNode *arg_tail;

	// Used for determining control flow
	// Base: 0
	int indent_level;

	// Whether this command was run with a dash ("-"), indicating to only
	// run it if the previous on this indent failed
	// Besides setting this to true, the dash will be ignored as a token
	bool else_flag;

	// The next command within the current pipeline
	// NULL if this is the last
	struct Command *next_pipeline;

	// Points to a variable name, that should be piped to and set, or NULL if
	// unused. Only the last Command in a pipeline will set this
	char *pipe_variable;

	// Point to a literal filename string if used. NULL otherwise
	char *redirect_read;
	char *redirect_write;
	char *redirect_append;
} Command;

void clear_command(Command *cmd);
Command *create_command();
void free_command(Command *cmd);

char **get_argv_array(Command *cmd);
void add_arg(Command *cmd, char *given_arg_name);
void dump_command(Command *cmd);

#endif // command_h_INCLUDED
