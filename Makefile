CC ?= gcc
CFLAGS ?= -O3 -Wall
LDFLAGS ?= -lm

SRC_DIR = src
BIN_DIR = bin

ifeq ($(OS),Windows_NT)
  EXE = .exe
  MKDIR = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
  RM = if exist $(BIN_DIR)\*$(EXE) del /q /f $(BIN_DIR)\*$(EXE)
else
  EXE =
  MKDIR = mkdir -p $(BIN_DIR)
  RM = rm -f $(BIN_DIR)/*
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
DEMOS = $(patsubst $(SRC_DIR)/%.c,%,$(SRCS))
TARGETS = $(patsubst %,$(BIN_DIR)/%$(EXE),$(DEMOS))

.PHONY: all clean help $(DEMOS)

all: $(TARGETS)

$(BIN_DIR)/%$(EXE): $(SRC_DIR)/%.c
	@$(MKDIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# Convenience target rules for each demo (e.g. `make cube_spinning`)
define DEMO_RULE
$(1): $(BIN_DIR)/$(1)$(EXE)
endef
$(foreach d,$(DEMOS),$(eval $(call DEMO_RULE,$(d))))

clean:
	$(RM)

help:
	@echo Gyro Terminal 3D Engine - Build System
	@echo ---------------------------------------
	@echo Usage:
	@echo   make              Build all demos into $(BIN_DIR)/
	@echo   make [demo]       Build a specific demo (e.g. make cube_spinning)
	@echo   make clean        Remove all built executables in $(BIN_DIR)/
	@echo   make help         Display this help message
	@echo.
	@echo Available demos:
	@echo   $(DEMOS)
