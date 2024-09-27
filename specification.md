# hisshi: (draft!) specification
This document contains some rough implementation notes. These are
subject to change, but they provide a base for some next steps in the
implementation.

# Philosophy
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