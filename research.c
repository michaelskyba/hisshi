#include <stdio.h>
#include <unistd.h>

int main() {
	int pid = fork();
	printf("There are two! I'm %d\n", pid);

	if (pid == 0) {
		printf("I'm the child, so I'm exec ing");
		execlp("date", "");
	}

	printf("Exiting normally. I'm %d\n", pid);
}
