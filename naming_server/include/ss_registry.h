// ss_registry.h
#ifndef SS_REGISTRY_H
#define SS_REGISTRY_H

#include "communication_protocols.h"

// Structure to hold Storage Server details

typedef struct StorageServer{
    char ss_ip[IP_LENGTH];
    // in this context, this is the port the storage server listens on for naming server requests
    int nm_port;
    // in this context, this is the port the storage server listens on for client requests
    int client_port;
    // Add other SS details as needed
    // Stores information of it's backup server
    // in this context, this is the port the storage server listens on for storage server requests
    int storage_server_port;
    int sock_fd;
    // This is the backup server's information for this storage server
    struct StorageServer *Backup;
    // Tells if the server has crashed
    int I_ded;
} StorageServer;

typedef struct
{
    StorageServer ** Storage_servers;
    int num_servers;
    int max_servers;
} Storage_server_list;
extern Storage_server_list myssl;
// Function to add a Storage Server to the registry
int add_storage_server(StorageServer ss);

// Function to remove a Storage Server from the registry
void remove_storage_server(const char* ss_ip, int ss_port);

// Function to retrieve Storage Server details
int get_storage_server(const char* ss_ip, int ss_port, StorageServer* ss);

// Add other registry-related function declarations as needed

#endif // SS_REGISTRY_H
