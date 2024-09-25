char *get_str_copy(char *base) {
	char *dest = (char *) malloc(strlen(base) + 1);
	strcpy(dest, base);
	return dest;
}

// Given name is not modified
char *get_bin_path(char *target) {
	// Absolute path
	if (*target == '/')
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
			if (strcmp(target, d->d_name) != 0)
				continue;

			int size = strlen(path_node) + strlen(target) + 1;
			char *bin_path = (char *) malloc(size);
			sprintf(bin_path, "%s/%s", path_node, target);
			return bin_path;
		}

		closedir(dir);

		// Read next section of PATH
		// Our local path_list copy is remembered by strtok
		path_node = strtok(NULL, ":");
	}

	// Found nothing. Give back original to spark the eventual execve error
	return get_str_copy(target);
}