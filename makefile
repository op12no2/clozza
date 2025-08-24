
CC := clang

.DEFAULT_GOAL := all

BUILD ?= release

ifeq ($(BUILD),release)
  CFLAGS  := -O3 -march=native -flto -fno-semantic-interposition
  LDFLAGS := -flto -fuse-ld=lld
else ifeq ($(BUILD),debug)
  CFLAGS  := -Og -g3 -Wall -Wextra -DDEBUG -fno-omit-frame-pointer
else
  $(error BUILD must be 'release' or 'debug')
endif

SRC := clozza.c
BIN := clozza

# --- rules ---
all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(BIN)

