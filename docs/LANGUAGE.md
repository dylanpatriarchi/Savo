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
savomax 3 8         # max(3.00, 8.00) = 8.00
savomin 3 8         # min(3.00, 8.00) = 3.00
savorandom 1 100    # a random integer in [1, 100]
```

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

## Full command list

| Command | Arguments | Effect |
|---------|-----------|--------|
| `savoprint` | `<"string">` or `<"string"> + <value>` | Print a string, optionally followed by a value |
| `savovar` | `<@name> <value>` | Define or update a variable |
| `savosum` | `<value> <value>` | Add |
| `savosubtract` | `<value> <value>` | Subtract |
| `savomoltiplication` | `<value> <value>` | Multiply |
| `savodivide` | `<value> <value>` | Divide |
| `savomod` | `<value> <value>` | Modulo |
| `savosqrt` | `<value>` | Square root |
| `savopow` | `<base> <exp>` | Power |
| `savoabs` | `<value>` | Absolute value |
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
