SRC=src
INC=include
OBJ=objects
BIN=bin

CC=gcc
CFLAGS = -I$(INC) -O0 -ggdb -Wall -lm

all:
	make $(OBJ)
	make $(BIN)
	make $(BIN)/libisp.a
	make $(BIN)/test

$(OBJ):
	mkdir $(OBJ)

$(BIN):
	mkdir $(BIN)

$(BIN)/lisp: $(SRC)/test.c
	$(CC) $(CFLAGS) $^ -L$(BIN) -lisp -o $@

$(BIN)/libisp.a: $(OBJ)/builtin.o \
$(OBJ)/data.o \
$(OBJ)/eval.o \
$(OBJ)/mem.o \
$(OBJ)/print.o \
$(OBJ)/read.o
	ar rcs $@ $^

$(OBJ)/builtin.o: $(SRC)/builtin.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ)/data.o: $(SRC)/data.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ)/eval.o: $(SRC)/eval.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ)/mem.o: $(SRC)/mem.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ)/print.o: $(SRC)/print.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJ)/read.o: $(SRC)/read.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean

clean:
	rm -rf $(OBJ)
	rm -rf $(BIN)
