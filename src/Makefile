CC = gcc

# -D_DEFAULT_SOURCE: Enable glibc dirent.h dir_type
# Used for ensuring you don't e.g. execute a directory, when reading PATH.
# Avoiding it is annoying and requires other syscalls

# CFLAGS = -g -O0 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -std=c99 -pedantic -fsanitize=address
# CFLAGS = -O2 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -std=c99 -pedantic
CFLAGS = -g -fsanitize=address

OBJ = builtin.o command.o exec.o function.o hash_table.o input_source.o main.o parser.o parse_state.o shell_state.o tokenizer.o util.o


hsh: $(OBJ)
	$(CC) $(CFLAGS) -o hsh $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o hsh
