# Savo language — build system
#
#   make            build the `savo` interpreter
#   make run        build, then start the interactive REPL
#   make example    build, then run examples/demo.savo
#   make clean      remove build artifacts and the binary

CC       := cc
CFLAGS   := -Wall -O2 -Isrc -Ibuild
LDLIBS   := -lm

SRC      := src
BUILD    := build
BIN      := savo

CORE_SRCS := $(SRC)/global.c $(SRC)/value.c $(SRC)/symtab.c $(SRC)/ast.c
GEN_SRCS  := $(BUILD)/parser.tab.c $(BUILD)/lex.yy.c

.PHONY: all run example test clean

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

# Bison generates both the parser and the token header used by the lexer.
$(BUILD)/parser.tab.c $(BUILD)/parser.tab.h: $(SRC)/parser.y | $(BUILD)
	bison -d -o $(BUILD)/parser.tab.c $(SRC)/parser.y

$(BUILD)/lex.yy.c: $(SRC)/lexer.l $(BUILD)/parser.tab.h | $(BUILD)
	flex -o $(BUILD)/lex.yy.c $(SRC)/lexer.l

$(BIN): $(GEN_SRCS) $(CORE_SRCS)
	$(CC) $(CFLAGS) $(GEN_SRCS) $(CORE_SRCS) -o $(BIN) $(LDLIBS)

run: $(BIN)
	./$(BIN)

example: $(BIN)
	./$(BIN) examples/demo.savo

test: $(BIN)
	@sh tests/run.sh

clean:
	rm -rf $(BUILD) $(BIN) savo.exe
