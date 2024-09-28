# hisshi: (draft!) specification
This document contains some rough implementation notes. These are
subject to change, but they provide a base for some next steps in the
implementation.

## Philosophy
hisshi makes design decisions centered around syntactical and functional
minimalism. When unclear, follow the Unix philosophy aggressively.

1. "Write programs that do one thing and do it well."
2. "Write programs to work together."
3. "Write programs to handle text streams, because that is a universal
interface."

The POSIX specification of a shell breaks rule 1 in "doing" far too much
itself, like sophisticated string parsing and arithmetic. These would be better
left to external programs, to the extent we can.

POSIX shells also do not reach far enough for the second. Eventually, it'd
create much room for extensibility if we incorporated
[Kakoune](https://github.com/mawww/kakoune)'s client-server model, allowing
external programs to interact with an active shell process.

Other, fancier shells break the third rule. hisshi sees it as virtuous, and
will commit to avoiding built-in handling of structured data like YAML or JSON.

> Wait, an even more universal interface might be direct, unstructured
binary...? What if we let variables hold completely arbitrary data, with only
some default text parsing, but with the option of leaving it raw? Then rule 2
can kick in and allow the same SQL etc. features of filtering and parsing when
binary programs pipe to each other?

## CLI usage
```
$ hsh myfile.sh
```

This means that shebangs are supported.

```
#!/usr/local/bin/hsh
echo test
```

Interactive usage is nontrivial to implement in a satisfying way. But I'd
ideally like it to work in the future.

## Regular commands, PATH
Status: Implemented

```
ls
/usr/bin/ls
../ls
```

Discounting the possibility of calling a function for now, we'll follow the
standard specification of taking the first token on a line to specify a program
to run. We'll support standard and absolute paths, as well as ordered lookups in
the `$PATH` variable.

## Control flow
Status: Planned

To execute a series of commands only given a `0` exit code of a previous
command one, you can use tab (spaces are unsupported) indentation.

```
false
	p "This will not be printed."

date | rg 2024
	p "Behold! The great year of Lord Samuel is upon us!"
	rm -rf $HOME
```

The dash character `-` is a special symbol used for branching on non-0 exit
codes. It functions as either an `elif` or an `else`, depending on whether you
provide another command to it.

```
date | rg 2024
	p "It is 2024."
- date | rg Sep
	p "It's not 2024, but it is September."
-
	p "It's not 2024, and it's not September. Instead, it is over."
```

The `-` character looks at the last statement made on the same line of
indentation. Nesting works as expected.

There are no explicit `if`, `elif`, or `else` keywords.

`&&` and `||` work mostly as expected, if you need similar behaviour on one
line.
