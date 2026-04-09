# Auto-detect a C compiler if CC is not set on the command line.
# Prefer clang if present, fall back to gcc, finally cc.
ifeq ($(origin CC),default)
  CC := $(shell command -v clang 2>/dev/null || command -v gcc 2>/dev/null || echo cc)
endif

CFLAGS   = -Wall -Wextra -std=c11 -Isrc -Iinclude -g -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -MMD -MP
LDFLAGS  = -lncurses
SRC      = $(wildcard src/*.c)
OBJ      = $(SRC:.c=.o)
DEP      = $(OBJ:.o=.d)
BIN      = camelot

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Auto-generated header dependencies (created by -MMD).
-include $(DEP)

debug: CFLAGS += -O0 -fsanitize=address,undefined
debug: LDFLAGS += -fsanitize=address,undefined
debug: clean $(BIN)

clean:
	rm -f src/*.o src/*.d $(BIN)

.PHONY: all clean debug
