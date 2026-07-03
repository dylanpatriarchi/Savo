# Savo

Savo is a tiny scripting language with a console REPL, implemented in C with
**Flex** (lexer) and **Bison** (parser). Every command is a keyword prefixed
with `savo` — `savoprint`, `savosum`, `savoif`, and so on — which makes short
scripts read almost like plain english.

```savo
savovar @answer 42
savoprint "The answer is " + @answer
savoif (@answer == 42)
```

```
The answer is 42.00
true
```

## Build

You need `flex`, `bison`, a C compiler and `make` (all standard on macOS and
Linux).

```sh
make            # build the ./savo interpreter
make run        # build, then start the interactive REPL
make example    # build, then run examples/demo.savo
make clean      # remove build artifacts
```

Generated sources and the binary land in `build/` and `./savo` and are ignored
by git.

## Usage

Start an interactive session:

```sh
./savo
```

Run a script file:

```sh
./savo examples/demo.savo
```

Inside the REPL, type `savohelp` for the full command list and `savoquit` (or
`savoexit`) to leave.

## Language at a glance

| Category    | Commands |
|-------------|----------|
| Output      | `savoprint` |
| Variables   | `savovar` |
| Arithmetic  | `savosum`, `savosubtract`, `savomoltiplication`, `savodivide`, `savomod` |
| Math        | `savosqrt`, `savopow`, `savoabs`, `savomax`, `savomin`, `savorandom` |
| Control     | `savoif`, `savofor`, `savowhile` |
| Console     | `savodir`, `savols`, `savocls`, `savoclear`, `savopointercell`, `savohelp`, `savoquit`, `savoexit` |

Any command that takes a **value** accepts either a number literal (including
negatives, e.g. `-17`) or a variable such as `@x`. See
[`docs/LANGUAGE.md`](docs/LANGUAGE.md) for the complete reference and more
examples.

### A quick tour

```savo
savovar @x 10           # define a variable
savosum @x 5            # -> 15.00
savorandom 1 6          # roll a die
savofor 3 "hello"       # print "hello" three times
savoif (@x > 3)         # -> true
savoprint "done\n"      # \n, \t, \r, \\ and \" escapes are supported
```

## Project layout

```
src/               language sources
├── lexer.l        Flex lexer (tokens, escapes, entry point)
├── parser.y       Bison grammar and command semantics
├── symtab.c/.h    variable storage
├── global.c/.h    shared runtime state
└── statements/    print / for / while helpers
examples/          runnable .savo scripts
docs/              language reference
Makefile           build system
```

## Contributing

Contributions are welcome — open an issue or reach the author at
`info@patriarchidylan.it`.
