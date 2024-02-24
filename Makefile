CC ?= gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4 poppler-glib)
LIBS = $(shell $(PKGCONFIG) --libs gtk4 poppler-glib)

SRC = main.c app.c win.c

OBJS = $(BUILT_SRC:.c=.o) $(SRC:.c=.o)

all: jumpdf

%.o: %.c
	$(CC) -c -o $(@F) $(CFLAGS) $<

jumpdf: $(OBJS)
	$(CC) -o $(@F) $(OBJS) $(LIBS)

clean:
	rm -f $(BUILT_SRC)
	rm -f $(OBJS)
	rm -f jumpdf
