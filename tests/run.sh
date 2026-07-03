#!/bin/sh
# Golden-file test runner: for each tests/*.savo, run it through ./savo and
# compare stdout against the matching tests/*.expected file.
#
# Trailing newlines are ignored (command substitution strips them), which keeps
# the comparison stable across script vs interactive newline handling.

cd "$(dirname "$0")/.." || exit 2

BIN=./savo
if [ ! -x "$BIN" ]; then
    echo "error: $BIN not built — run 'make' first" >&2
    exit 2
fi

pass=0
fail=0
for t in tests/*.savo; do
    exp_file="${t%.savo}.expected"
    got=$("$BIN" "$t" 2>/dev/null)
    exp=$(cat "$exp_file" 2>/dev/null)
    if [ "$got" = "$exp" ]; then
        echo "PASS  $t"
        pass=$((pass + 1))
    else
        echo "FAIL  $t"
        echo "  --- expected ---"
        printf '%s\n' "$exp" | sed 's/^/  /'
        echo "  --- got ---"
        printf '%s\n' "$got" | sed 's/^/  /'
        fail=$((fail + 1))
    fi
done

echo "-------------------------"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
