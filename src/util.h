#pragma once

#define DEBUG 0

#define debug(fmt, ...) \
	do { if (DEBUG) fprintf(stderr, "%-20s " fmt, \
		 __func__, __VA_ARGS__); } while (0)

#define debug_raw(fmt, ...) \
	do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

/*
#define debug(fmt, ...) \
	do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
		__LINE__, __func__, __VA_ARGS__); } while (0)
*/

char *get_str_copy(char *base);
char *copy_with_newline(char *base);

char *get_bin_path(char *target);
