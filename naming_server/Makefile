# Makefile

CC = gcc
CFLAGS = -Wall -g -fsanitize=address -pthread -I./include  # Add the path to the header files here
SRC = src/main.c src/nm_functions.c src/nm_bookkeeping.c src/trie.c  src/cache.c
OBJ = $(SRC:.c=.o)
TARGET = naming_server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
