// main.c
#ifndef MACRO
#define MACRO
#include "naming_server.h"
#include "nm_bookkeeping.h"
#include "trie.h"
#include "ss_registry.h"
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "uthash.h"
#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#endif

struct trie* files;
Storage_server_list myssl;

int main(int argc, char *argv[]) 
{
    if (argc != 3) {
        printf("Usage: %s <public_ip> <public_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* public_ip = argv[1];
    int public_port = atoi(argv[2]);

    // Initialize logging
    init_nm_logging("naming_server.log");

    //Creating the trie for the naming server.
    files = trie_create();
    myssl.num_servers = 0;
    myssl.max_servers = 100;
    myssl.Storage_servers = (StorageServer**)calloc(myssl.max_servers,sizeof(StorageServer*));    // Initialize Naming Server
    for (int i = 0; i < myssl.max_servers; i++)
    {
        myssl.Storage_servers[i] = NULL; 
    }
    
    initialize_nm(public_ip, public_port);

    return EXIT_SUCCESS;
}