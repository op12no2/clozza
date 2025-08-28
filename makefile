# --- toolchain ---
CC := clang

# --- layout ---
SRC_DIR   := src
BUILD_DIR := build
BIN       := clozza

# --- build type: release|debug (default release) ---
BUILD ?= release

ifeq ($(BUILD),release)
  CFLAGS  := -O3 -march=native -flto -fno-semantic-interposition -MMD -MP
  LDFLAGS := -flto -fuse-ld=lld
else ifeq ($(BUILD),debug)
  CFLAGS  := -Og -g3 -Wall -Wextra -DDEBUG -fno-omit-frame-pointer -MMD -MP
  LDFLAGS :=
else
  $(error BUILD must be 'release' or 'debug')
endif

# --- sources/objects/deps ---
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))
DEP := $(OBJ:.o=.d)

# --- default goal ---
.DEFAULT_GOAL := all
.PHONY: all clean

all: $(BIN)

# Link in root
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

# Compile to build/, search headers in src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)/
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

# Ensure build dir exists
$(BUILD_DIR)/:
	mkdir -p $@

# Include auto-generated dependencies
-include $(DEP)

clean:
	rm -rf $(BUILD_DIR) $(BIN)

