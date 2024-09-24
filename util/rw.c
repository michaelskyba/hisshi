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

void tail(int start) {
	// 4: "tail", "-n", offset, NULL
	char **argv = (char **) malloc(sizeof(char *) * 4);
	argv[0] = tail_bin;
	argv[1] = "-n";

	argv[2] = (char *) malloc(ARG_SIZE);
	sprintf(argv[2], "%d", start);

	argv[3] = NULL;

	char *env[] = {NULL};

	execve(tail_bin, argv, env);
}

int main() {
	tail(10);
}
