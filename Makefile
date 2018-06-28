CFLAGS =
objects := he.o editor.o buff.o term.o utils.o action.o

all: he
he: $(objects)

debug: CFLAGS += -g -Og
debug: all

clean:
	$(RM) $(objects) he
