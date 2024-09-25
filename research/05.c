#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
	char *path = getenv("PATH");
	printf("-1\n");
	char *x = strtok(path, ":");

	printf("0\n");

	// printf("%p\n", (void *) (x - path));
	// printf("%p\n", (void *) x);
	printf("%s\n", x);
}
