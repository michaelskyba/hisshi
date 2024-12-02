#include <stdio.h>
#include <stdlib.h>

int main() {
	// int x = atoi("123");
	// int x = atoi("a123");
	int x = atoi("-123");
	printf("%d\n", x);
	perror("foo\n");
}
