#ifndef command_h_INCLUDED
#define command_h_INCLUDED

struct ArgNode {
	char *name;
	struct ArgNode *next;
};

struct Command {
	char *path;

	// The arg list and argc include the $0 name
	int argc;
	struct ArgNode *arg_head;
	struct ArgNode *arg_tail;

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
};

void clear_command(struct Command *cmd);
struct Command *create_command();
void free_command(struct Command *cmd);

char **get_argv_array(struct Command *cmd);
void add_arg(struct Command *cmd, char *given_arg_name);
void dump_command(struct Command *cmd);

#endif // command_h_INCLUDED
