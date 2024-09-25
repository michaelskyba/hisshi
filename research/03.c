#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <sys/wait.h>

#undef stdin
#define stdin 0

int main() {
	int pipe_fds[2];
	pipe(pipe_fds);

	int status;
	char *s = malloc(100);
	status = read(stdin, s, 3);
	printf("Read %s, with status %d\n", s, status);

	char s2[100] = {0};
	sprintf(s2, "(This is s2. Originally read %s, with status %d.)", s, status);

	write(pipe_fds[1], s2, strlen(s2));

	status = read(pipe_fds[0], s, 20);
	printf("From pipe_fds[0], we found: |%s|, with status %d\n", s, status);
}
