SRC=src
INC=include
OBJ=objects
BIN=bin

CC=gcc
CFLAGS = -I$(INC) -O0 -ggdb -Wall
OBJS=$(SRC)/builtin.o \
	$(SRC)/data.o \
	$(SRC)/eval.o \
	$(SRC)/mem.o \
	$(SRC)/print.o \
	$(SRC)/read.o \
	$(SRC)/thread.o

LDFLAGS=-lm

.PHONY: all clean

all: $(BIN)/libisp.a $(BIN)/lisp $(BIN)/sample

$(BIN)/lisp: $(SRC)/repl.c $(BIN)/libisp.a
	$(CC) -o $@ $^ $(CFLAGS) -pthread $(LDFLAGS)

$(BIN)/sample: $(SRC)/sample.c $(BIN)/libisp.a
	$(CC) -o $@ $^ $(CFLAGS) -pthread $(LDFLAGS)

$(BIN)/libisp.a: $(OBJS)
	ar rcs $@ $^

.c.o:
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(OBJS)
	rm -f $(BIN)/*
