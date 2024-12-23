# 必死 (ひっし)
hisshi: an unsafe shell with minimal syntax, aiming to sacrifice script
readability in exchange for short-term ergonomics

### Project status
hisshi is currently somewhat usable, and can replace around half of my existing
POSIX-compliant scripts, though the lack of features would make certain
operations more, rather than less verbose. The lack of subshells, for instance,
requires always piping to a variable on a separate line.

Interactive usage is technically possible to try, using
```
$ hsh /dev/stdin
```

I plan to further implement the rest of my ideas as well as write more detailed
documentation regarding the design. For now, [this
issue](https://github.com/michaelskyba/hisshi/issues/1) is being used as a
makeshift tracker.

The short, now-outdated specification can be found [here](./specification.md).
