CFLAGS=-std=gnu99 -Wall

BINARY=usamba
SOURCES = usamba.c comm.c chipid.c eefc.c
OBJS = $(SOURCES:.c=.o)

$(BINARY): $(OBJS)

clean:
	@rm -f $(OBJS) $(BINARY)
