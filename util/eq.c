#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: echo val1 | %s val2\n", argv[0]);
		return 1;
	}

	char buffer[BUFFER_SIZE];
	if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
		// Error or no input
		return 1;
	}

	// Remove trailing newline if present
	// Could warrant a flag in the future but is fine as the default for now
	int len = strlen(buffer);
	if (len > 0 && buffer[len - 1] == '\n')
		buffer[len - 1] = '\0';

	// Compare with the CLI argument
	if (strcmp(buffer, argv[1]) == 0)
		return 0;
	else
		return 1;
}
