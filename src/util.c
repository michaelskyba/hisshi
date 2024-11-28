// Assuming -fsanitize=address
#define STACKTRACE	char *stacktrace_tmp = malloc(1); *(stacktrace_tmp + 5) = 1

// TODO Make dynamic
char getc_buffer[128];
char *getc_buffer_p = getc_buffer;

// Assumes you will only ever read from the same file
char getcb(FILE *f) {
	return (getc_buffer_p > getc_buffer) ? *(--getc_buffer_p) : getc(f);
}
void ungetcb(char c) {
	*(getc_buffer_p++) = c;
}

char *get_str_copy(char *base) {
	char *dest = malloc(strlen(base) + 1);
	strcpy(dest, base);
	return dest;
}

// Given name is not modified
char *get_bin_path(char *target) {
	// Absolute or relative paths
	if (*target == '/' || *target == '.')
		return get_str_copy(target);

	// We need a copy because strtok modifies your input
	// If we modify what getenv returns, it actually updates the variable
	char *path_list = get_str_copy(getenv("PATH"));
	char *path_node = strtok(path_list, ":");

	while (path_node != NULL) {
		DIR *dir = opendir(path_node);

		// Usually because the directory doesn't exist
		if (!dir) {
			perror(path_node);
			continue;
		}

		// Iterate through files in this path_node dir to look for target
		// matches
		struct dirent *d = NULL;
		while ((d = readdir(dir)) != NULL) {
			bool valid_file = d->d_type == DT_LNK || d->d_type == DT_REG;
			if (strcmp(target, d->d_name) != 0 || !valid_file)
				continue;

			// +2: /, \0
			char *bin_path = malloc(strlen(path_node) + strlen(target) + 2);
			sprintf(bin_path, "%s/%s", path_node, target);

			closedir(dir);
			free(path_list);

			return bin_path;
		}

		closedir(dir);

		// Read next section of PATH
		// Our local path_list copy is remembered by strtok
		path_node = strtok(NULL, ":");
	}

	free(path_list);

	// Found nothing. Give back original to spark the eventual execve error
	return get_str_copy(target);
}
