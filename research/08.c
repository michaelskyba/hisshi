#include <stdio.h>

int main() {
	int x = -1;
	int *y = (int *) x;
	printf("%d %p\n", x, (void *)y);
}
