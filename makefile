# --- toolchain ---
CC := clang

# --- default goal ---
.DEFAULT_GOAL := all

# --- build type: release|debug (default release) ---
BUILD ?= release

ifeq ($(BUILD),release)
  CFLAGS  := -O3 -march=native -flto -fno-semantic-interposition
  LDFLAGS := -flto -fuse-ld=lld
else ifeq ($(BUILD),debug)
  # Og keeps code “real” while still debuggable; sanitizers catch the nasties.
  CFLAGS  := -Og -g3 -Wall -Wextra -DDEBUG \
             -fsanitize=address,undefined,leak \
             -fno-omit-frame-pointer \
             -fno-sanitize-recover=all
  LDFLAGS := -fsanitize=address,undefined,leak
else
  $(error BUILD must be 'release' or 'debug')
endif

# --- single source ---
SRC := clozza.c
BIN := clozza

# --- rules ---
all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(BIN)

