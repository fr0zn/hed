git_hash := $(shell git rev-parse --verify HEAD)
version := $(shell git describe --long 2>/dev/null || echo "1.0.0 (no git)")
version_short := $(shell git describe --abbrev=0 2>/dev/null || echo "1.0.0")

EXE = hed

SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS += -Iinclude -DHED_GIT_HASH=\"$(git_hash)\"
CPPFLAGS += -DHED_VERSION=\"$(version)\" -DHED_VERSION_SHORT=\"$(version_short)\"
CFLAGS += -Wall

.PHONY: all clean

all: $(EXE)

debug: CFLAGS += -g -DDEBUG
debug: $(EXE)

install:
	@install -m 755 $(EXE) /usr/local/bin
	@install -m 644 hed.1 /usr/local/share/man/man1

$(EXE): $(OBJ)
	@$(CC) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJ)
	@$(RM) $(EXE)
