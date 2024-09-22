#include <stdio.h>
#include <stdlib.h>

void panic(char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

int main(int argc, char **argv) {
	if (argc < 2)
		panic("Usage: hsh script-file");

	FILE *script_file = fopen(argv[1], "r");

	int chunk_size = 100;
	char line[chunk_size];

	while (fgets(line, chunk_size, script_file))
		printf("%s", line);
}
