#define HASH_BUCKETS 1024
#define HASH_ROLL_CONSTANT 37

int hash_str(char *str) {
	int hash_val = 0;
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		hash_val = hash_val * HASH_ROLL_CONSTANT + str[i];

	return hash_val % HASH_BUCKETS;
}
