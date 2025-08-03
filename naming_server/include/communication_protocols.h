#ifndef COMMUNICATION_PROTOCOLS_H
#define COMMUNICATION_PROTOCOLS_H

#define MAX_PATH_LENGTH 256
#define MAX_DATA_SIZE 1024
#define IP_LENGTH 16
#define MESSAGE_LENGTH 512
#define PERMISSION_LEN 12

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
    //the path of the file the client wants to read/write/create/delete, doesnt have to be sent by client.
    char path[MAX_PATH_LENGTH];
    char data[MAX_DATA_SIZE];
    int flags; // e.g., synchronous or asynchronous
    //WILL ONLY BE USED FOR COPY
    char dest_ip[IP_LENGTH];
    //WILL ONLY BE USED FOR COPY
    int dest_port;
} Message;

// Not to be confused with Filedata. This has permissions and size and not lock.
typedef struct {
    char permition[PERMISSION_LEN];
    long int file_size;
}FileData2;

// Structure for Responses
typedef struct {
    StatusCode status;
    char message[MESSAGE_LENGTH];
    char ss_ip[IP_LENGTH];
    int ss_port;
} Response;
typedef struct {
    CommandType command;
    char srcpath[MAX_PATH_LENGTH];
    char destpath[MAX_DATA_SIZE];
    char dest_ip[IP_LENGTH];
    int dest_port;
}CopyMessage;

#endif // COMMUNICATION_PROTOCOLS_H