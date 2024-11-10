FROM gcc AS hisshi-build
WORKDIR /usr/local/src
COPY src ./

# TODO Makefile
RUN gcc -g -O0 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -std=c99 -pedantic main.c -o hsh

FROM debian AS hisshi-run
WORKDIR /usr/local/src
COPY --from=hisshi-build /usr/local/src/hsh ./
COPY examples .
CMD ["./hsh", "./20"]
