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

### Automated tests
Unit tests are mostly a waste of time in the context of a shell like this. If it
performs your desired operations under the edge cases you give it, then
everything is working correctly. Modifying them for every internal refactor is
low ROI.

Instead, we use automated integration tests with a simple Python script:
```
cd util
./auto_test
```
This reads from `./tests/config.txt` to find a list of test hisshi scripts to
run, and then the correct stdout/stderr we expect them to have.

They assume your compiled `hsh` is in production mode and that you've installed
the shipped rc/init. If you used `./build`, both of these are the default.
