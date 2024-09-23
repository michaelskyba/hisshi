#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

void execute(char *cmd) {
	int pid = fork();

	if (pid < 0) {
		perror(cmd);
		exit(1);
	}

	if (pid == 0) {
		char *argv[] = {NULL};
		char *env[] = {NULL};

		printf("%d: execve(%s)\n", getpid(), cmd);
		execve(cmd, argv, env);

		// execve only returns control to us if it fails
		perror(cmd);
		exit(1);
	}

	printf("%d-%d: Time to wait\n", getpid(), pid);
	int status = 0;
	int id = wait(&status);
	printf("%d-%d: Done waiting on %d with status %d\n", getpid(), pid, id, status);
}

int main() {
	printf("Starting with PID %d\n", getpid());

	FILE *script_file = fopen("./input.txt", "r");
	fgetc(script_file);

	printf("offset before: %ld\n", lseek(3, 0, SEEK_CUR));
	execute("foo");
	printf("offset after: %ld\n", lseek(3, 0, SEEK_CUR));
}
