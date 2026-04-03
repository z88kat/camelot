CC       = clang
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
