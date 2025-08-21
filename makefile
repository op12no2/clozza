# --- toolchain ---
CC := clang

# --- default goal ---
.DEFAULT_GOAL := all

# --- build type: release|debug (default release) ---
BUILD ?= release

ifeq ($(BUILD),release)
  CFLAGS := -O3 -flto -march=native 
  LDFLAGS := -flto -fuse-ld=lld
else ifeq ($(BUILD),debug)
  CFLAGS := -O0 -g -Wall -Wextra -DDEBUG \
            -fsanitize=address,undefined,leak \
            -fno-omit-frame-pointer
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

