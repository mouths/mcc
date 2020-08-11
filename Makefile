SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = mcc
CC = gcc
TEST = test.sh
TESTDIR = ./test/
CFLAGS = -g -MMD -MP

.PHONY: all clean test

test: $(TEST) $(TARGET)
	./$(TEST)

all: $(TARGET)

$(TARGET):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rdf $(TARGET) *.o $(OUT) *.d
	rm -rdf $(TESTDIR)*.S $(TESTDIR)*.out $(TESTDIR)*.dSYM/ $(TESTDIR)*.o
