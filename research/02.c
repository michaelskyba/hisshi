/*
For me prints

Starting with PID 1051326
offset before: 24
1051326-1051327: Time to wait
1051327: execve(foo)
foo: No such file or directory
1051326-1051327: Done waiting on 1051327 with status 256
offset after: 1
*/

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

    FILE *script_file = fopen("../examples/10-broken", "r");
    if (!script_file) {
        perror("fopen");
        exit(1);
    }

    fgetc(script_file);

    int fd = fileno(script_file);
    if (fd == -1) {
        perror("fileno");
        exit(1);
    }

    printf("offset before: %ld\n", lseek(fd, 0, SEEK_CUR));
    execute("foo");
    printf("offset after: %ld\n", lseek(fd, 0, SEEK_CUR));

    fclose(script_file);
}

