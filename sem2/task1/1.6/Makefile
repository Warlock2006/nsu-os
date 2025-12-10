CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -fsanitize=address -fsanitize=leak
CFLAGS += -Iinclude -std=c11 -fPIC
LDFLAGS = -fsanitize=address -fsanitize=leak

SRC_DIR = src
INCLUDE_DIR = include
TESTS_DIR = tests

SRC = $(SRC_DIR)/mythreads.c
OBJ = $(SRC:.c=.o)
LIB = libmythreads.so

TEST_SRC = $(TESTS_DIR)/tests.c
TEST_BIN = tests/test_all

all: $(LIB) $(TEST_BIN)

$(LIB): $(OBJ)
	$(CC) -shared -fPIC -o $@ $^ $(LDFLAGS)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): $(TEST_SRC) $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -lmythreads $(LDFLAGS)

test: $(TEST_BIN)
	LD_LIBRARY_PATH=. ./$(TEST_BIN)

clean:
	rm -f $(LIB) $(OBJ) $(TEST_BIN)

.PHONY: all clean test