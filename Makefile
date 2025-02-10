# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
INCLUDES = -I.
LDFLAGS = -L/usr/local/lib
LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    LDLIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
    EXT = .exe
else
    ifeq ($(UNAME_S),Darwin)
        LDLIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation
    else
        LDLIBS = -lraylib -lGL -lm -lpthread -ldl -lrt
    endif
    EXT =
endif

# Directories
EXAMPLE_DIR = example
BIN_DIR = bin

# Files
EXAMPLE_SRC = $(wildcard $(EXAMPLE_DIR)/*.c)
EXAMPLE_TARGETS = $(patsubst $(EXAMPLE_DIR)/%.c,$(BIN_DIR)/%$(EXT),$(EXAMPLE_SRC))

# Create directories
$(shell mkdir -p $(BIN_DIR))

# Main target
all: $(EXAMPLE_TARGETS)

# Rule to compile each example
$(BIN_DIR)/%$(EXT): $(EXAMPLE_DIR)/%.c tree2d.h tree3d.h
	@echo "Compiling $< into $@"
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(LDLIBS)

# Clean
clean:
	@echo "Cleaning up..."
	rm -rf $(BIN_DIR)/*

# Rebuild
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild