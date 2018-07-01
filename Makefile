EXE :=  he
SRC :=  $(shell find . -maxdepth 1 -type f -name '*.c')
OBJ :=  $(SRC:.c=.o)
LIBS =

# Preprocessor flags here
CPPFLAGS    :=  -I.
# Compiler flags here
CFLAGS      :=  -std=c99 -Wall -Werror-implicit-function-declaration

.PHONY: all clean

all: CPPFLAGS += -DNDEBUG
all: $(EXE)

debug: CPPFLAGS += -DDEBUG
debug: CFLAGS += -g
debug: $(EXE)

$(EXE): $(OBJ)
	@echo "Compilation complete!"
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(LIBS) -o $@
	@echo "Linking complete!"

clean:
	@$(RM) $(EXE) $(OBJ) *.dSYM
	@echo "Cleanup complete!"
