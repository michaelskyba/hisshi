#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

struct arg_node {
	char *name;
	struct arg_node *next;
};

struct command {
	char *path;

	int argc;
	struct arg_node *arg_head;
	struct arg_node *arg_tail;
};

void panic(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

char **get_argv_array(struct command *cmd) {
	// +1: Room for NULL
	int len = cmd->argc + 1;

	char **argv = (char **) malloc(sizeof(char *) * len);
	struct arg_node *p = cmd->arg_head;

	for (int i = 0; i < cmd->argc; i++) {
		argv[i] = p->name;
		p = p->next;
	}

	argv[cmd->argc] = NULL;
	return argv;
}

void execute(struct command *cmd) {
	int pid = fork();

	if (pid < 0) {
		perror(cmd->path);
		exit(1);
	}

	if (pid == 0) {
		// TODO Think about whether you need to free the command struct
		// once you're executing it

		char *env[] = {NULL};

		printf("%d: execve(%s)\n", getpid(), cmd->path);
		execve(cmd->path, get_argv_array(cmd), env);

		// execve only returns control to us if it fails
		perror(cmd->path);
		_exit(1);
	}

	printf("%d-%d: Starting wait()\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done wait() on %d. Rec status %d\n", getpid(), pid, id, status);
}

struct command *create_command() {
	struct command *cmd = (struct command *) malloc(sizeof(struct command));
	cmd->path = NULL;
	cmd->argc = 0;
	cmd->arg_head = NULL;
	cmd->arg_tail = NULL;

	return cmd;
}

void add_arg(struct command *cmd, char *arg_name) {
	struct arg_node *arg = (struct arg_node *) malloc(sizeof(struct arg_node));
	arg->name = arg_name;
	arg->next = NULL;

	if (cmd->argc++ == 0)
		cmd->arg_head = arg;
	else
		cmd->arg_tail->next = arg;

	cmd->arg_tail = arg;
}

void parse_line(char *line) {
	if (*line == '\0') {
		printf("Skipping blank line\n");
		return;
	}

	if (*line == '#') {
		printf("Skipping comment: %s\n", line);
		return;
	}

	struct command *cmd = create_command();
	cmd->path = line;
	add_arg(cmd, cmd->path); // Convention: set name as $0

	// Test
	add_arg(cmd, "foo");
	add_arg(cmd, "bar");

	execute(cmd);
}

void parse_script(FILE *script_file) {
	// TODO address lines longer than chunk_size
	int chunk_size = 100;
	char *line = (char *) malloc(sizeof(char) * chunk_size);
	char *p = line;

	while ((*p = getc(script_file)) != EOF) {
		if (*p != '\n') {
			p++;
			continue;
		}

		*p = '\0';
		p = line;

		parse_line(line);
	}
}

int main(int argc, char **argv) {
	if (argc != 2)
		panic("Usage: hsh script-file");

	printf("Starting on PID %d\n", getpid());
	printf("%s: received file %s\n", argv[0], argv[1]);

	FILE *script_file = fopen(argv[1], "r");
	parse_script(script_file);
}
