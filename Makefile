CFLAGS =
objects := he.o editor.o buff.o term.o utils.o

all: he
he: $(objects)

debug: CFLAGS += -ggdb -Og
debug: all

clean:
	$(RM) $(objects) he
