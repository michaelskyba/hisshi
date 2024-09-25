#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define head_bin "/usr/bin/uu-head"
#define tail_bin "/usr/bin/uu-tail"

#define ARG_SIZE 100

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

int main() {
	until(-10);
}
