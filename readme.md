# Savo

Savo is a tiny scripting language with a console REPL, implemented in C with a
**hand-written lexer and recursive-descent parser** (no external toolchain).
Every command is a keyword prefixed with `savo` — `savoprint`, `savosum`,
`savoif`, and so on — which makes short scripts read almost like plain english.

```savo
savovar @answer = 6 * 7
savoprint "The answer is " + @answer
savoif (@answer == 42)
    savoprint "correct!\n"
savoend
```

```
The answer is 42.00
correct!
```

## Build

You only need a C compiler and `make` (standard on macOS and Linux) — there is
no flex/bison dependency.

```sh
make            # build the ./savo interpreter
make run        # build, then start the interactive REPL
make example    # build, then run examples/demo.savo
make test       # build, then run the golden-file test suite
make asan       # build with AddressSanitizer + UBSan, then run the tests
make clean      # remove build artifacts
```

The `./savo` binary is ignored by git.

## Usage

```sh
./savo                     # start the interactive REPL
./savo examples/demo.savo  # run a script file
./savo -e 'savosum 2 3'    # evaluate code inline
echo 'savosum 2 3' | ./savo -   # read a script from stdin
./savo --version           # print the version
./savo --help              # print usage
```

The [`examples/`](examples) directory has more, including `trees.savo` (a
recursive tree with sum/height) and `graphs.savo` (adjacency-list DFS and BFS)
built entirely from objects and arrays.

When stdin is not a terminal (a pipe or a file), Savo runs in quiet script mode:
no banner and no auto-inserted newlines. Inside the REPL, type `savohelp` for the
full command list and `savoquit` (or `savoexit`) to leave. A syntax error reports
its line number and the interpreter keeps going with the next line.

## Language at a glance

| Category    | Commands |
|-------------|----------|
| Output      | `savoprint` |
| Variables   | `savovar` |
| Arithmetic  | `savosum`, `savosubtract`, `savomoltiplication`, `savodivide`, `savomod` |
| Math        | `savosqrt`, `savopow`, `savoabs`, `savofloor`, `savoceil`, `savoround`, `savolog`, `savolog10`, `savomax`, `savomin`, `savorandom` |
| Strings     | `savolen`, `savoupper`, `savolower`, `savostr`, `savonum`, `savotrim`, `savosubstr`, `savoindexof`, `savoreplace`, `savosplit`, `savojoin` (+ `+` concatenation) |
| Arrays      | `[ … ]` literals, `@a[i]` indexing, `savopush`, `savopop`, `savoset`, `savolen`, `savocontains` |
| Objects     | `{ k: v }` literals, `@o.field` / `@o["k"]` access, `savoset`, `savolen`, `savokeys`, `savocontains` |
| Control     | `savoif`/`savoelif`/`savoelse`/`savoend`, `savowhile`, `savofor`, `savoforeach` |
| Input       | `savoinput` |
| Functions   | `savodef`/`savoreturn` |
| Console     | `savodir`, `savols`, `savocls`, `savoclear`, `savopointercell`, `savohelp`, `savoquit`, `savoexit` |

Values are **dynamically typed** — a variable holds a number, a string, an array
or an object. Anywhere a value is expected you can write a full **expression**:
`+ - * / %`, comparisons, `&& || !`, parentheses, subscripts `@a[i]`, field access
`@o.field` and nested function calls such as `savosqrt(@x * @x + 9)`. `+` adds
numbers and **concatenates** when either side is a string. Arrays and objects
are mutable and shared by reference, so together they express records, trees and
graphs. See [`docs/LANGUAGE.md`](docs/LANGUAGE.md) for the complete reference.

### A quick tour

```savo
savovar @x = (3 + 4) * 2      # expressions with precedence -> 14
savoprint "x = " + @x + "\n"  # strings concatenate with +
savovar @who = "world"
savoprint savoupper("hi " + @who) + "\n"   # HI WORLD

savovar @nums = [3, 1, 4]     # arrays
savopush @nums 1
savoprint "nums has " + savolen(@nums) + " items\n"

savodef fact(@n)              # recursive function
    savoif (@n <= 1)
        savoreturn 1
    savoend
    savoreturn @n * fact(@n - 1)
savoend
savoprint "5! = " + fact(5)   # 120.00
```

## Project layout

```
src/               language sources
├── lexer.c/.h     hand-written lexer (tokens, escapes, streaming input)
├── parser.c/.h    recursive-descent parser that builds the AST
├── main.c         CLI entry point (argument handling, REPL/script setup)
├── ast.c/.h       AST nodes + tree-walking interpreter
├── value.c/.h     dynamic value type (number / string / array / object)
├── symtab.c/.h    scoped variable storage
└── global.c/.h    shared runtime state
examples/          runnable .savo scripts
tests/             golden-file test suite (make test)
docs/              language reference
Makefile           build system
```

## Contributing

Contributions are welcome — open an issue or reach the author at
`info@patriarchidylan.it`.
