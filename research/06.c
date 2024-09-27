#include <stdio.h>
#include <stdlib.h>

int main() {
	struct foo {
		int x;
	};

	struct foo *bar = malloc(sizeof(struct foo));
	bar->x = 5;
	printf("%d\n", bar->x);
}
