# Auto-detect a C compiler if CC is not set on the command line.
# Prefer clang if present, fall back to gcc, finally cc.
ifeq ($(origin CC),default)
  CC := $(shell command -v clang 2>/dev/null || command -v gcc 2>/dev/null || echo cc)
endif

CFLAGS   = -Wall -Wextra -std=c11 -Isrc -Iinclude -g
LDFLAGS  = -lncurses
SRC      = $(wildcard src/*.c)
OBJ      = $(SRC:.c=.o)
BIN      = camelot

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o $(BIN)

.PHONY: all clean
