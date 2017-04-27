CC=gcc --std=c11 -Ofast
CFLAGS=-I. -pg $(shell pkg-config --cflags glib-2.0)
DEPS = bwtsearch.h
OBJ_E = bwtsearch.o
LIBS = $(shell pkg-config --libs glib-2.0)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: bwtsearch 

bwtsearch: $(OBJ_E)
	gcc --std=c11 -o $@ $^ $(CFLAGS) $(LIBS)
