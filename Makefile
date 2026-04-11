# Auto-detect a C compiler if CC is not set on the command line.
# Prefer clang if present, fall back to gcc, finally cc.
ifeq ($(origin CC),default)
  CC := $(shell command -v clang 2>/dev/null || command -v gcc 2>/dev/null || echo cc)
endif

CFLAGS   = -Wall -Wextra -std=c11 -Isrc -Iinclude -g -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -MMD -MP
LDFLAGS  = -lncurses

# --- Optional SDL2 support (auto-detected) ---
# Try sdl2-config first, then pkg-config, then common Homebrew paths.
SDL2_CONFIG := $(shell command -v sdl2-config 2>/dev/null)
ifdef SDL2_CONFIG
  SDL2_CFLAGS  := $(shell sdl2-config --cflags) -DHAS_SDL2
  SDL2_LDFLAGS := $(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf
else ifneq ($(wildcard /opt/homebrew/include/SDL2/SDL.h),)
  SDL2_CFLAGS  := -I/opt/homebrew/include/SDL2 -DHAS_SDL2
  SDL2_LDFLAGS := -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lSDL2_ttf
else ifneq ($(wildcard /usr/local/include/SDL2/SDL.h),)
  SDL2_CFLAGS  := -I/usr/local/include/SDL2 -DHAS_SDL2
  SDL2_LDFLAGS := -L/usr/local/lib -lSDL2 -lSDL2_image -lSDL2_ttf
endif
ifdef SDL2_CFLAGS
  CFLAGS  += $(SDL2_CFLAGS)
  LDFLAGS += $(SDL2_LDFLAGS)
endif

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
