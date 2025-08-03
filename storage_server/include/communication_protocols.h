#ifndef COMMUNICATION_PROTOCOLS_H
#define COMMUNICATION_PROTOCOLS_H

#define MAX_PATH_LENGTH 256
#define MAX_DATA_SIZE 1024
#define IP_LENGTH 16
#define MESSAGE_LENGTH 512
#define PERMISSION_LEN 11

// Command Types
typedef enum {
    CMD_REGISTER_SS,
    CMD_READ,
    CMD_WRITE,
    CMD_CREATE,
    CMD_DELETE,
    CMD_LIST,
    CMD_STREAM,
    CMD_COPY_SEND,
    CMD_COPY_RECV,
    CMD_GET_INFO
    // Add other command types as needed
} CommandType;

// Response Status
typedef enum {
    STATUS_SUCCESS,
    STATUS_ERR_FILE_NOT_FOUND,
    STATUS_ERR_WRITE_CONFLICT,
    STATUS_ERR_INVALID_COMMAND,
    // Add other status codes as needed
    STATUS_DONE,
} StatusCode;

// Structure for Messages
typedef struct {
    CommandType command;
    char path[MAX_PATH_LENGTH];
    char data[MAX_DATA_SIZE];
    int flags; // e.g., synchronous or asynchronous
     //WILL ONLY BE USED FOR COPY
    char dest_ip[IP_LENGTH];
    //WILL ONLY BE USED FOR COPY
    int dest_port;
} Message;

typedef struct {
    char permission[PERMISSION_LEN];
    ssize_t file_size;
}FileData2;

// Structure for Responses
typedef struct {
    StatusCode status;
    char message[MESSAGE_LENGTH];
    char ss_ip[IP_LENGTH];
    int ss_port;
} Response;

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
    // This is the backup server's information for this storage server
    struct StorageServer *Backup;
    // Tells if the server has crashed
    int I_ded;
} StorageServer;

typedef struct
{
    // server IP and Port
    char *ip; 
    int port; 
    
    // number of files in the storage server
    int dir_len; 

    // list of files in the storage server
    char **files; 
} SSData;

#endif // COMMUNICATION_PROTOCOLS_H