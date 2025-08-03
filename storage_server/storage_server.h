#ifndef STORAGE_SERVER_H
#define STORAGE_SERVER_H

#include <string.h>


typedef struct  
{
    char *path;
    int data;
    int file_size;
    int file_id;
    int is_open;
    int is_locked;
} File;

typedef struct 
{
    int num;
} StorageServer;

int storage_server_init(StorageServer *ss, int num);
int storage_server_read(StorageServer *ss, int *data);
int storage_server_write(StorageServer *ss, int data);
int storage_server_destroy(StorageServer *ss);




#endif // STORAGE_SERVER_H