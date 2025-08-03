# Makefile

CC = gcc
CFLAGS = -Wall -g -pthread -I./include  # Add the path to the header files here
SRC = src/storage_server.c
BACKUP_SRC = src/backup_server.c
OBJ = $(SRC:.c=.o)
BACKUP_OBJ = $(BACKUP_SRC:.c=.o)
TARGET = storage_server
BACKUP_TGT = backup_server

all: $(TARGET) $(BACKUP_TGT)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BACKUP_TGT): $(BACKUP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) $(BACKUP_OBJ) $(BACKUP_TGT) 	 	

