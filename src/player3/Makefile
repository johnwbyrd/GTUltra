#==============================================================================
# Makefile for Player3 C Implementation
#==============================================================================
# This Makefile builds the player3 library for the Commodore 64 using the
# llvm-mos compiler toolchain.
#
# USAGE:
#   make            - Build release version
#   make debug      - Build debug version (with assertions and logging)
#   make test       - Build and run tests
#   make clean      - Remove all build artifacts
#   make help       - Show this help message
#
# REQUIREMENTS:
#   - llvm-mos-sdk must be installed and in PATH
#   - See: https://llvm-mos.org/wiki/Getting_Started
#==============================================================================

#------------------------------------------------------------------------------
# Compiler and Tools
#------------------------------------------------------------------------------

# llvm-mos C compiler
CC = mos-c64-clang

# Archiver (for creating .a library files)
AR = mos-c64-ar

# Compiler flags
CFLAGS = -std=c11 -Wall -Wextra -Werror
CFLAGS += -I include

# Optimization flags (release build)
OPTFLAGS = -O2

# Debug flags
DEBUGFLAGS = -g -DPLAYER_DEBUG

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
TEST_DIR = test

#------------------------------------------------------------------------------
# Source Files
#------------------------------------------------------------------------------

# Implementation files
SRCS = \
    $(SRC_DIR)/player3.c \
    $(SRC_DIR)/sequencer.c \
    $(SRC_DIR)/effects.c \
    $(SRC_DIR)/wavetable.c \
    $(SRC_DIR)/pulsetable.c \
    $(SRC_DIR)/filtertable.c \
    $(SRC_DIR)/soundfx.c

# Header files (for dependency tracking)
HEADERS = \
    $(INC_DIR)/sid.h \
    $(INC_DIR)/player3_types.h \
    $(INC_DIR)/player3.h

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test files
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)

#------------------------------------------------------------------------------
# Output Files
#------------------------------------------------------------------------------

# Library output
LIB = $(BUILD_DIR)/libplayer3.a

# Example programs
EXAMPLE_DIR = examples
EXAMPLES = \
    $(BUILD_DIR)/simple_player.prg \
    $(BUILD_DIR)/sfx_demo.prg

#------------------------------------------------------------------------------
# Build Rules
#------------------------------------------------------------------------------

.PHONY: all clean debug test build-tests check help examples

# Default target: build release library
all: $(LIB)

# Build debug version
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean $(LIB)

# Build tests (compile only - tests are C64 binaries)
build-tests: CFLAGS += $(DEBUGFLAGS)
build-tests: $(TEST_BINS)
	@echo "Tests built successfully!"
	@echo "Note: Tests are C64 binaries. Use 'make test' with x64sc emulator to run them."

# Run tests (requires VICE emulator)
test: build-tests
	@echo "Running tests with x64sc emulator..."
	@if ! command -v x64sc >/dev/null 2>&1; then \
		echo "Error: x64sc emulator not found. Install VICE to run tests."; \
		echo "Tests were compiled successfully but cannot run without emulator."; \
		exit 1; \
	fi
	@for test in $(TEST_BINS); do \
		echo "  Running $$test..."; \
		x64sc -console $$test || exit 1; \
	done
	@echo "All tests passed!"

# Check syntax and compilation (CI-friendly)
check: CFLAGS += $(DEBUGFLAGS)
check: $(LIB) build-tests
	@echo "✓ All source files compiled successfully!"

# Build example programs
examples: $(EXAMPLES)

# Create library from object files
$(LIB): $(OBJS) | $(BUILD_DIR)
	@echo "Creating library: $@"
	$(AR) rcs $@ $^

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) $(OPTFLAGS) -c -o $@ $<

# Build test executables
$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(LIB) | $(BUILD_DIR)
	@echo "Building test: $@"
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILD_DIR) -lplayer3

# Build example programs
$(BUILD_DIR)/%.prg: $(EXAMPLE_DIR)/%.c $(LIB) | $(BUILD_DIR)
	@echo "Building example: $@"
	$(CC) $(CFLAGS) $(OPTFLAGS) -o $@ $< -L$(BUILD_DIR) -lplayer3

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Help message
help:
	@echo "Player3 C Implementation - Makefile Help"
	@echo ""
	@echo "Targets:"
	@echo "  make            - Build release library (libplayer3.a)"
	@echo "  make debug      - Build debug library with assertions"
	@echo "  make check      - Compile everything (CI-friendly)"
	@echo "  make build-tests- Build test executables (compile only)"
	@echo "  make test       - Build and run tests (requires VICE emulator)"
	@echo "  make examples   - Build example programs"
	@echo "  make clean      - Remove all build artifacts"
	@echo "  make help       - Show this message"
	@echo ""
	@echo "Requirements:"
	@echo "  - llvm-mos-sdk installed (https://llvm-mos.org)"
	@echo "  - mos-c64-clang in PATH"
	@echo "  - x64sc emulator (optional, for running tests)"
	@echo ""
	@echo "Output:"
	@echo "  Library: $(LIB)"
	@echo "  Objects: $(BUILD_DIR)/*.o"
	@echo "  Tests:   $(BUILD_DIR)/test_*"
	@echo "  Examples: $(BUILD_DIR)/*.prg"

#------------------------------------------------------------------------------
# Dependencies
#------------------------------------------------------------------------------

# All object files depend on all headers (conservative, but safe)
$(OBJS): $(HEADERS)

#------------------------------------------------------------------------------
# Installation (optional)
#------------------------------------------------------------------------------

PREFIX ?= /usr/local
INSTALL_LIB_DIR = $(PREFIX)/lib/mos-c64
INSTALL_INC_DIR = $(PREFIX)/include/player3

.PHONY: install uninstall

install: $(LIB)
	@echo "Installing player3 library..."
	mkdir -p $(INSTALL_LIB_DIR)
	mkdir -p $(INSTALL_INC_DIR)
	cp $(LIB) $(INSTALL_LIB_DIR)/
	cp $(HEADERS) $(INSTALL_INC_DIR)/
	@echo "Installation complete!"
	@echo "  Library: $(INSTALL_LIB_DIR)/libplayer3.a"
	@echo "  Headers: $(INSTALL_INC_DIR)/"

uninstall:
	@echo "Uninstalling player3 library..."
	rm -f $(INSTALL_LIB_DIR)/libplayer3.a
	rm -rf $(INSTALL_INC_DIR)
	@echo "Uninstallation complete!"

#==============================================================================
# End of Makefile
#==============================================================================
