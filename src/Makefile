CC = gcc

# Expects that you have env vars set:
# - CFLAGS
# (See the root `build` script)

OBJ = builtin.o command.o exec.o hash_table.o input_source.o main.o parser.o parse_state.o shell_state.o tokenizer.o tokenize_func.o util.o


hsh: $(OBJ)
	$(CC) ${CFLAGS} -o hsh $(OBJ)

%.o: %.c
	$(CC) ${CFLAGS} -c $< -o $@

clean:
	rm -f *.o hsh
