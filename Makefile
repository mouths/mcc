SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = mcc
CC = gcc
TEST = test.sh
CFLAGS = -MMD -MP

.PHONY: all clean

test: $(TEST) $(TARGET)
	./$(TEST)

all: $(TARGET)

$(TARGET):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	rm -rdf $(TARGET) *.o *.out $(OUT) *.S *.dSYM/ *.d
