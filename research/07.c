#include <stdio.h>
#include <unistd.h>

int main() {
	printf("%d\n", STDIN_FILENO);
	printf("%d\n", STDOUT_FILENO);
	printf("%d\n", STDERR_FILENO);
}
