# Savo language — build system
#
#   make            build the `savo` interpreter
#   make run        build, then start the interactive REPL
#   make example    build, then run examples/demo.savo
#   make test       build, then run the golden-file test suite
#   make asan       build with AddressSanitizer + UBSan, then run the tests
#   make clean      remove build artifacts and the binary
#
# The lexer and parser are hand-written C — no flex/bison toolchain required.

CC       := cc
CFLAGS   := -Wall -Wextra -O2 -Isrc
LDLIBS   := -lm

SRC      := src
BIN      := savo

SRCS     := $(SRC)/main.c $(SRC)/lexer.c $(SRC)/parser.c \
            $(SRC)/global.c $(SRC)/value.c $(SRC)/symtab.c $(SRC)/ast.c
HEADERS  := $(wildcard $(SRC)/*.h)

.PHONY: all run example test asan clean

all: $(BIN)

$(BIN): $(SRCS) $(HEADERS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN) $(LDLIBS)

run: $(BIN)
	./$(BIN)

example: $(BIN)
	./$(BIN) examples/demo.savo

test: $(BIN)
	@sh tests/run.sh

# Build with sanitizers and run the suite; catches memory errors and UB.
asan: CFLAGS := -Wall -Wextra -g -fsanitize=address,undefined -Isrc
asan: clean $(BIN)
	@sh tests/run.sh

clean:
	rm -rf build
	rm -f $(BIN) savo.exe
