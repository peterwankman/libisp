SRC=src
INC=include
OBJ=objects
BIN=bin

CC=gcc
CFLAGS = -I$(INC) -O0 -ggdb -Wall -lm
OBJS=$(SRC)/builtin.o \
	$(SRC)/data.o \
	$(SRC)/eval.o \
	$(SRC)/mem.o \
	$(SRC)/print.o \
	$(SRC)/read.o \
	$(SRC)/thread.o

.PHONY: all clean

all: $(BIN)/libisp.a $(BIN)/lisp

$(BIN)/lisp: $(SRC)/test.c $(BIN)/libisp.a
	$(CC) $(CFLAGS) -L$(BIN) -lisp -pthread -o $@ $^

$(BIN)/libisp.a: $(OBJS)
	ar rcs $@ $^
.c.o:
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -rf $(OBJS) $(BIN)/libisp.a $(BIN)/lisp
