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

	FILE *f = fopen("./myfile.txt", "w");
	int file_fd = fileno(f);

	close(file_fd); // Works without closing too, though
	dup2(pipe_fds[1], file_fd);

	fprintf(f, "(This is from the fprintf. Originally read %s, with status %d.)\n", s, status);
	fflush(f);

	status = read(pipe_fds[0], s, 40);
	printf("From pipe_fds[0], we found: |%s|, with status %d\n", s, status);
}
