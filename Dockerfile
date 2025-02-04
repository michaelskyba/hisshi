FROM gcc AS hisshi-build
WORKDIR /usr/local/src
COPY src ./

// TODO Add a separate recipe with CFLAGS for avoiding -fsanitize
RUN make

FROM debian AS hisshi-run
WORKDIR /usr/local/src
COPY --from=hisshi-build /usr/local/src/hsh ./
COPY util/tests .
CMD ["./hsh", "./20"]
