#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37
#define TARGET 69

const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int charset_size = sizeof(charset) - 1; // -1: exclude \0

int hash_str(char *str) {
    int hash_val = 0;
    int len = strlen(str);

    for (int i = 0; i < len; i++)
        hash_val = hash_val * HASH_ROLL_CONSTANT + str[i];

    return hash_val % HASH_BUCKETS;
}

// Function to increment the charset indices for the current string
// Return:
// 1: successfully incremented
// 0: all combinations completed
int increment_indices(int *indices, int length) {
    for (int i = length - 1; i >= 0; i--) {
        if (indices[i] >= charset_size - 1)
	        continue;

        indices[i]++;
        for (int j = i + 1; j < length; j++)
            indices[j] = 0;

        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atol(argv[1]);
    printf("Searching for %d strings whose hashes are %d\n", n, TARGET);

    int found = 0;

    // Starting length of strings
    int length = 1;

    // Current string, including \0
    // Start with length 1 and expands later
    char *current_str = NULL;

    while (found < n) {
        current_str = malloc((length + 1) * sizeof(char));

        // Initialize all charset indices to 0
        int *indices = calloc(length, sizeof(int));

        int has_more = 1;
        while (has_more && found < n) {
            // Build the current string based on indices
            for (int i = 0; i < length; i++)
                current_str[i] = charset[indices[i]];

            current_str[length] = '\0';

            if (hash_str(current_str) == TARGET) {
                printf("%s\n", current_str);

                if (++found >= n)
                    break;
            }

            // Increment the indices to get the next string
            has_more = increment_indices(indices, length);
        }

        free(indices);
        free(current_str);

        // Move to the next string length
        length++;
    }

    return 0;
}
