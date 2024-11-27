// fileno() from stdio.h relies on POSIX standard
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
	// if stdin is connected to a terminal, assume no input is provided
	if (isatty(fileno(stdin)))
		return 1;

	// We assume a blank input is still non-empty, which is fine for now, but
	// may also warrant a flag later
	return fgetc(stdin) == EOF ? 1 : 0;
}
