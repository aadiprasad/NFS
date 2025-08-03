# Makefile

CC = gcc
CFLAGS = -Wall -pthread -g -fsanitize=address -I./include  # Add the path to the header files here
SRC = src/main.c src/linked-list.c  # src/parse.c
OBJ = $(SRC:.c=.o)
TARGET = client

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
