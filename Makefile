# Savo language — build system
#
#   make            build the `savo` interpreter
#   make run        build, then start the interactive REPL
#   make example    build, then run examples/demo.savo
#   make test       build, then run the golden-file test suite
#   make asan       build with AddressSanitizer + UBSan, then run the tests
#   make lib        build libsavo.a (embeddable interpreter, see src/savo.h)
#   make clean      remove build artifacts and the binary
#
# The lexer and parser are hand-written C — no flex/bison toolchain required.

CC       := cc
CFLAGS   := -Wall -Wextra -O2 -Isrc
LDLIBS   := -lm

SRC      := src
BUILD    := build
BIN      := savo
LIB      := libsavo.a

# Core objects make up the embeddable library; main.o is the CLI only.
CORE     := lexer parser global value symtab ast savo
CORE_OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(CORE)))
MAIN_OBJ := $(BUILD)/main.o
HEADERS  := $(wildcard $(SRC)/*.h)

.PHONY: all run example test asan lib embed clean

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC)/%.c $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(CORE_OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(CORE_OBJS) $(MAIN_OBJ) -o $(BIN) $(LDLIBS)

lib: $(LIB)
$(LIB): $(CORE_OBJS)
	ar rcs $(LIB) $(CORE_OBJS)

# Build the embedding demo against the library (see examples/embed.c).
embed: $(LIB)
	$(CC) $(CFLAGS) examples/embed.c $(LIB) $(LDLIBS) -o embed

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
	rm -rf $(BUILD) $(BIN).dSYM
	rm -f $(BIN) savo.exe $(LIB) embed
