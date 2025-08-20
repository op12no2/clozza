# --- toolchain ---
CC := clang

# --- default goal ---
.DEFAULT_GOAL := all

# --- build type: release|debug (default release) ---
BUILD ?= release

ifeq ($(BUILD),release)
  CFLAGS := -O3 -flto -fno-exceptions -fomit-frame-pointer -march=x86-64-v3 -DNDEBUG
  LDFLAGS := -flto -fuse-ld=lld
else ifeq ($(BUILD),debug)
  CFLAGS := -O0 -g -Wall -Wextra
  LDFLAGS :=
else
  $(error BUILD must be 'release' or 'debug')
endif

# --- single source ---
SRC := clozza.c
BIN := clozza

# --- rules ---
all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -march=native $< -o $@ $(LDFLAGS)

clean:
	rm -f $(BIN)

