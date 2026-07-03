# The Savo Language Reference

Savo is a line-oriented scripting language. Each command starts with a keyword
prefixed by `savo` and is followed by its arguments on the same conceptual line.
Keywords are **case-insensitive** (`savoprint`, `SavoPrint` and `SAVOPRINT` are
all valid).

## Table of contents

- [Lexical elements](#lexical-elements)
- [Values and variables](#values-and-variables)
- [Output](#output)
- [Arithmetic](#arithmetic)
- [Math functions](#math-functions)
- [Control flow](#control-flow)
- [Console commands](#console-commands)
- [Command-line interface](#command-line-interface)
- [Error handling](#error-handling)
- [Full command list](#full-command-list)

## Lexical elements

| Element     | Description | Examples |
|-------------|-------------|----------|
| Number      | Integer or decimal, optionally negative. Stored as a float. | `42`, `3.14`, `-17`, `.5` |
| String      | Double-quoted. Supports `\n`, `\t`, `\r`, `\\`, `\"` escapes. | `"hello\n"` |
| Variable    | An `@`-prefixed identifier. | `@x`, `@total` |
| Comment     | `#` to end of line; ignored. | `# a note` |

Whitespace and blank lines are insignificant.

## Values and variables

A **value** is anywhere Savo expects a number: it is either a number literal or
a variable reference. Variables are created and updated with `savovar` and hold
a single floating-point number.

```savo
savovar @x 10          # define @x = 10
savovar @x 25          # reassign @x = 25
savovar @y @x          # copy: @y = 25
savosum @x @y          # -> 50.00
```

Referencing an undefined variable prints a warning to stderr and evaluates to
`0`, so a script keeps running rather than crashing.

## Output

```savo
savoprint "hello world\n"          # print a string literal
savoprint "score: " + @x           # print a string followed by a value
savoprint @x                       # print a number or variable directly
```

In interactive mode `savoprint` appends a newline automatically; when running a
file it does not, so add `\n` yourself where you want line breaks.

## Arithmetic

Each takes two values and prints the result with two decimals.

```savo
savosum 15 25              # 40.00
savosubtract 50 20         # 30.00
savomoltiplication 6 7     # 42.00
savodivide 84 2            # 42.00
savomod 17 5              # 2.00
```

Division and modulo by zero print an error to stderr instead of crashing.

## Math functions

```savo
savosqrt 144        # √144.00 = 12.00
savopow 2 10        # 2.00^10.00 = 1024.00
savoabs -9          # 9.00
savofloor 3.7       # 3.00
savoceil 3.2        # 4.00
savoround 3.5       # 4.00
savolog 2.71828     # 1.0000  (natural logarithm)
savolog10 1000      # 3.0000  (base-10 logarithm)
savomax 3 8         # max(3.00, 8.00) = 8.00
savomin 3 8         # min(3.00, 8.00) = 3.00
savorandom 1 100    # a random integer in [1, 100]
```

`savolog` and `savolog10` report an error on stderr for non-positive input.

`savorandom` swaps its bounds if given in reverse order, so `savorandom 100 1`
works too.

## Control flow

### Conditionals

`savoif` evaluates a condition and prints `true` or `false`.

```savo
savoif (10 == 10)          # true
savoif (@x != 0)           # compare with a variable
savoif (5 < 8)             # < > <= >= == != are all supported
savoif (@x)                # truthy test: true when @x != 0
savoif (!0)                # negation: true when the value is 0
savoif ("savo" == "savo")  # string equality (== and !=)
```

### Loops

Repeat a string a fixed number of times:

```savo
savofor 3 "hi"      # prints hi three times
savowhile 3 "hi"    # same, bounded repeat
```

C-style counted loop `(start, end, step)` — iterates while `i < end`:

```savo
savofor (0, 5, 1) "row"              # print "row" 5 times
savofor (0, 5, 1) "index: " + 0      # append the loop counter
savofor (1, 4, 1) "double: " * 2     # append counter * 2
```

Loop bounds accept variables too: `savofor (1, @n, 1) "..."`.

## Console commands

```savo
savodir             # list files in the current directory (alias: savols)
savols "*.savo"     # list files matching an argument
savocls             # clear the screen (Windows)
savoclear           # clear the screen (Linux/macOS)
savopointercell "s" # print the memory address of a string
savohelp            # show the built-in command summary
savoquit            # exit (alias: savoexit)
```

## Command-line interface

```sh
savo                     # interactive REPL (when stdin is a terminal)
savo script.savo         # run a script file
savo -e '<code>'         # evaluate code passed on the command line
savo -                   # read a script from stdin
savo -h | --help         # usage
savo -v | --version      # version
```

When stdin is a pipe or a file, Savo runs in **quiet script mode**: it prints no
banner and does not auto-insert newlines after `savoprint`. When stdin is a
terminal it runs the interactive REPL with the banner and `>>>` prompt.

## Error handling

- A **syntax error** prints `savo: line N: syntax error` to stderr and the
  interpreter resumes at the next line, so a single typo does not abort a run.
- **Undefined variables** warn on stderr and evaluate to `0`.
- **Division / modulo by zero** and **log of a non-positive number** warn on
  stderr and produce no result line.
- The process **exit status** is `0` on success, `1` if any error was reported,
  and `2` for a bad command-line invocation.

## Full command list

| Command | Arguments | Effect |
|---------|-----------|--------|
| `savoprint` | `<"string">`, `<value>`, or `<"string"> + <value>` | Print a string and/or a value |
| `savovar` | `<@name> <value>` | Define or update a variable |
| `savosum` | `<value> <value>` | Add |
| `savosubtract` | `<value> <value>` | Subtract |
| `savomoltiplication` | `<value> <value>` | Multiply |
| `savodivide` | `<value> <value>` | Divide |
| `savomod` | `<value> <value>` | Modulo |
| `savosqrt` | `<value>` | Square root |
| `savopow` | `<base> <exp>` | Power |
| `savoabs` | `<value>` | Absolute value |
| `savofloor` | `<value>` | Round down |
| `savoceil` | `<value>` | Round up |
| `savoround` | `<value>` | Round to nearest |
| `savolog` | `<value>` | Natural logarithm |
| `savolog10` | `<value>` | Base-10 logarithm |
| `savomax` | `<value> <value>` | Maximum |
| `savomin` | `<value> <value>` | Minimum |
| `savorandom` | `<min> <max>` | Random integer in range |
| `savoif` | `(<condition>)` | Compare / test, prints `true`/`false` |
| `savofor` | `<count> <"str">` or `(a,b,s) <"str">` | Repeat / counted loop |
| `savowhile` | `<count> <"str">` | Bounded repeat |
| `savodir` / `savols` | `[argument]` | List files |
| `savocls` / `savoclear` | — | Clear screen |
| `savopointercell` | `<"string">` | Print a memory address |
| `savohelp` | — | Show command summary |
| `savoquit` / `savoexit` | — | Exit |
