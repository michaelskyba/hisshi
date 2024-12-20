#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

#undef stdin
#define stdin 0

#undef stdout
#define stdout 1

#define head_bin "/usr/bin/uu-head"
#define tail_bin "/usr/bin/uu-tail"

#define ARG_SIZE 100

enum {
	ARG_START,
	arg_end,
};

enum {
	HEAD,
	TAIL
};

/*
tail supports n:, -n:
head supports :n, :-n

It seems more useful to need to start
at n and then go further (tail --> head)
rather than start at n and go backwards
(head --> tail)

rw supports
	:b
	a:
	a:b
	n (n:1)
	"" (:10)
*/

void coreutil(char *name, char *arg) {
	char **argv = (char **) malloc(sizeof(char *) * 4);
	argv[0] = name;
	argv[1] = "-n";
	argv[2] = arg;
	argv[3] = NULL;

	char *env[] = {NULL};
	execve(tail_bin, argv, env);

	fprintf(stderr, "failed: %s -n %s\n", name, arg);
	perror(name);
}

// start from and including start
// 1-indexed positively
// -1: last char
void from(int start) {
	char *arg = (char *) malloc(ARG_SIZE);
	sprintf(arg, "%+d", start);
	coreutil(tail_bin, arg);
}

// start at 0 and go until and including (end)
// -0: last char
// So until(-1) will skip the last char
// Sounds jank / inconsistent from tail, but works fine in practice
void until(int end) {
	char *arg = (char *) malloc(ARG_SIZE);
	sprintf(arg, "%d", end);
	coreutil(head_bin, arg);
}

void coreutil_child(int type, int pipe_fd[2], int arg) {
	int pid = fork();

	if (pid < 0) {
		perror("rw: fork failed");
		exit(1);
	}

	// Parent continues to next
	if (pid > 0)
		return;

	int read = pipe_fd[0];
	int write = pipe_fd[1];

	// tail masks stdout with (write), but continues reading from stdin
	if (type == TAIL) {
		// Don't touch head's read
		close(read);

		close(stdout);
		dup2(write, stdout);

		from(arg);
	}

	// head masks stdin with (read), but continueus writing to stdout
	if (type == HEAD) {
		// Don't touch tail's write
		close(write);

		close(stdin);
		dup2(read, stdin);

		until(arg);
	}
}

void from_until(int start, int end) {
	int pipe_fd[2];

	if (pipe(pipe_fd) == -1) {
		perror("rw: pipe failed");
		exit(1);
	}

	coreutil_child(TAIL, pipe_fd, start);
	coreutil_child(HEAD, pipe_fd, end);

	close(stdin);
	close(stdout);
	close(pipe_fd[0]);
	close(pipe_fd[1]);

	while (wait(NULL) > 0) ;
}

int main(int argc, char **argv) {
	// head -10 as default
	if (argc < 2)
		until(10);

	// 3: We don't support "rw filename" for default -10
	if (argc == 3) {
		int file_fd = open(argv[2], O_RDONLY);
		close(stdin);
		dup2(file_fd, stdin);
	}

	int start = 0;
	int end = -0; // relative to output of start
	int state = ARG_START;

	char *arg = argv[1];
	char *content = arg;
	char *p = arg - 1;

	while (*++p) {
		if (!isdigit(*p) && *p != '-' && *p != ':') {
			printf("usage: rw [start]:[end] [filename]\n");
			return 1;
		}

		if (*p == ':') {
			*p = '\0';
			if (p != arg)
				start = atoi(content);

			state = arg_end;
			content = p+1;
		}
	}

	// We never hit any colon, so we were just given one number
	// Take this to mean n:1
	if (state == ARG_START) {
		start = atoi(content);
		end = 1;
	}

	// We hit a colon, indicating the start of the end
	if (state == arg_end)
		end = atoi(content);

	if (start == 0) until(end);
	if (end == 0) from(start);

	from_until(start, end);
}
