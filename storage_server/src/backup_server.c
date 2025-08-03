#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include "communication_protocols.h"
#include<libgen.h>

#include <sys/types.h>
#include <dirent.h>
#include <ftw.h>
// #include <ss_registry.h>

#include "file_structure.h"

#define MAX_FILES 2000
#define MAX_PATH 256
#define NOPENFD 20
#define MAX_CLIENTS 100 

char *ss_ip;
int ss_port;

char *dir;
int len_files;
char **files;
FileData *file_datas;

void handle_client_rw(int client_socket);

int ss_socket = -1;

void initialize_server_socket() {
    int opt = 1;
    
    // Create the socket once
    ss_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ss_socket < 0) {
        perror("socket");
        exit(1);
    }
    
    if (setsockopt(ss_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("socket option");
        exit(1);
    }
    
    struct sockaddr_in ss_addr;
    memset(&ss_addr, 0, sizeof(struct sockaddr_in));
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(ss_port);
    ss_addr.sin_addr.s_addr = inet_addr(ss_ip);
    
    if (bind(ss_socket, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    if (listen(ss_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(1);
    }
    
    printf("Storage Server is listening on %s:%d\n", ss_ip, ss_port);
}

// @return relative path
char *rename_path1(const char *curr_dir)
{
    int len_h = (int)strlen(dir);
    int len_c = (int)strlen(curr_dir);
    int len_r = len_c; // max length of rel_path

    char *rel_path = (char*)calloc(MAX_PATH + 3,sizeof(char));
    rel_path[len_r] = 0;

    // checking if the current directory contains the home directory in its pathname
    if (len_c < len_h)
    {
        strcpy(rel_path, curr_dir);
        return rel_path;
    }
    for (int i = 0; i < len_h; i++)
    {
        if (curr_dir[i] != dir[i])
        {
            strcpy(rel_path, curr_dir);
            return rel_path;
        }
    }

    // replace
    len_r = len_c - len_h; // updated max length possible
    // rel_path[0] = '~';

    int i;
    for (i = len_h; i < len_c; i++)
    {
        rel_path[i - len_h] = curr_dir[i];
    }
    rel_path[i - len_h] = 0;
    return rel_path;
}

// @return absolute path
char *rename_path2(char *curr_dir)
{
    int len_h = (int)strlen(dir);
    int len_c = (int)strlen(curr_dir);
    int len_a = len_h + len_c;

    char *abs_path = malloc(len_a + 1);
    abs_path[len_a] = 0;

    // // checking if the current directory contains '~' in its pathname
    // if (curr_dir[0] != '~') {
    //     strcpy(abs_path, curr_dir);
    //     return abs_path;
    // }

    // replace
    // int i;
    for (int i = 0; i < len_h; i++)
    {
        abs_path[i] = dir[i];
    }
    for (int i = 0; i < len_c; i++)
    {
        abs_path[i + len_h] = curr_dir[i];
    }
    char *temp = curr_dir;

    return abs_path;
}

// @return index in FileData array. -1 if not found
int search(char *path)
{
    printf("Input path: %s\n", path); 
    for (int i = 0; i < len_files; i++)
    {
        printf("File %d: %s\n",i,file_datas[i].path);
        if (strcmp(file_datas[i].path, path) == 0)
        {
            return i;
        }
    }
    return -1;
}

int process_entry(const char *path, const struct stat *sb, int type, struct FTW *ftwbuf)
{
    // if (type == FTW_F)
    {
        if (strcmp("/storage_server", rename_path1(path)))
        {
            files[len_files++] = rename_path1(path);

            // for file_datas
            // iterator
            int i = len_files - 1;
            //  pathname
            file_datas[i].path = rename_path1(path);
            //  lock
            if (pthread_rwlock_init(&(file_datas[i].lock), NULL) != 0)
            {
                perror("could not initialise rwlock");
            }
            file_datas[i].metadata = NULL; // for now; probably forever
            file_datas[i].type = type == FTW_D; // 0 if file, 1 if directory
            printf("filename %s filetype %d\n", file_datas[i].path, file_datas[i].type); 
        }

        // x[0] = path;
        // ss.files[ss.dir_len++] = malloc(MAX_PATH);
        // strcpy(ss.files[ss.dir_len], path);
    }
    return 0;
}

int search_files()
{
    dir = malloc(256);
    getcwd(dir, 256);

    // ss.dir_len = 0;
    files = (char **)malloc(sizeof(char *) * MAX_FILES);
    file_datas = (FileData *)calloc(MAX_FILES, sizeof(FileData));
    len_files = 0;

    if (nftw(dir, &process_entry, NOPENFD, FTW_DEPTH | FTW_PHYS) == -1)
    {
        perror("File tree walk failed");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// void *handle_nm (void *args) {
//     int ss_socket = *((int *) args);
//     printf("socket: %d\n", ss_socket);

//     struct sockaddr_in nm_addr;
//     socklen_t nm_addr_len = sizeof(nm_addr);
//     int nm_socket = accept(ss_socket, (struct sockaddr *)&nm_addr, &nm_addr_len);
//     if (nm_socket < 0) {
//         perror("accept");
//         exit(1);
//     }

// }

// #define MAX_CLIENTS 10

void *wait_for_client_connection_rw(void *arg)
{
    int ss_socket = *((int *)arg);

    // Listen for incoming connections, create a thread to handle each connection
    // Accept the connection
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(ss_socket, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_socket < 0)
    {
        perror("accept");
        exit(1);
    }
    printf("Connection accepted from client\n");
    handle_client_rw(client_socket);
    return NULL; 
}

// char* read_file(char* path) {
//     FILE* file = fopen(path, "r");
//     if (file == NULL) {
//         printf("Could not open file: %s\n", path);
//         return NULL;
//     }

//     // Get file size
//     fseek(file, 0, SEEK_END);
//     long file_size = ftell(file);
//     rewind(file);

//     // Allocate memory for string (+1 for null terminator)
//     char* content = (char*)malloc(file_size + 1);
//     if (content == NULL) {
//         printf("Memory allocation failed\n");
//         fclose(file);
//         return NULL;
//     }

//     // Read file content
//     size_t read_size = fread(content, 1, file_size, file);
//     if (read_size != file_size) {
//         printf("Failed to read entire file\n");
//         free(content);
//         fclose(file);
//         return NULL;
//     }

//     // Add null terminator
//     content[file_size] = '\0';

//     fclose(file);
//     return content;
// }

#define CHUNK 4096

void receive_file(int sock, char *output_path)
{
    long file_size;
    recv(sock, &file_size, sizeof(file_size), 0);

    FILE *file = fopen(output_path, "wb");
    unsigned char *buffer = malloc(CHUNK);
    long total_received = 0;

    while (total_received < file_size)
    {
        size_t to_read = (file_size - total_received < CHUNK) ? file_size - total_received : CHUNK;
        ssize_t bytes_received = recv(sock, buffer, to_read, 0);
        if (bytes_received <= 0)
            break;

        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }

    free(buffer);
    fclose(file);
}

ssize_t send_file(int sock, char *path)
{
    printf("path %s\n",path);
    FILE *file = fopen(path, "rb");
    printf("opened file\n");
    if (file == NULL)
    {
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("filesize: %ld\n", file_size);

    // Send file size first
    if (send(sock, &file_size, sizeof(file_size), 0) < 0)
    {
        perror("file size not sent\n");
        fclose(file);
        return -1;
    }

    unsigned char *buffer = malloc(CHUNK);
    if (buffer == NULL)
    {
        fclose(file);
        return -1;
    }

    size_t bytes_read;
    ssize_t total_sent = 0;

    while ((bytes_read = fread(buffer, 1, CHUNK, file)) > 0)
    {
        ssize_t bytes_sent = send(sock, buffer, bytes_read, 0);
        if (bytes_sent < 0)
        {
            perror("not sent\n");
            printf("%ld bytes of file after byte %ld not sent\n", bytes_sent, total_sent);
            //free(buffer);
            fclose(file);
            return -1;
        }
        total_sent += bytes_sent;
    }
    printf("total sent: %ld\n", total_sent);
    free(buffer);
    fclose(file);
    return total_sent;
}

FileData2 *get_file_data(char* addr)
{
    FileData2 *data = malloc(sizeof(FileData2));
    struct stat sb;
    int success = stat(addr,&sb);
    data->file_size = sb.st_size;
    data->permission[0] = (sb.st_mode & S_IFDIR) ? 'd' : '-';
    data->permission[1] = (sb.st_mode & S_IRUSR) ? 'r' : '-';
    data->permission[2] = (sb.st_mode & S_IWUSR) ? 'w' : '-';
    data->permission[3] = (sb.st_mode & S_IXUSR) ? 'x' : '-';
    data->permission[4] = (sb.st_mode & S_IRGRP) ? 'r' : '-';
    data->permission[5] = (sb.st_mode & S_IWGRP) ? 'w' : '-';
    data->permission[6] = (sb.st_mode & S_IXGRP) ? 'x' : '-';
    data->permission[7] = (sb.st_mode & S_IROTH) ? 'r' : '-';
    data->permission[8] = (sb.st_mode & S_IWOTH) ? 'w' : '-';
    data->permission[9] = (sb.st_mode & S_IXOTH) ? 'x' : '-';
    data->permission[10] = '\0';
    return data;
}

ssize_t send_file_data(int sock, char *addr)
{
    FileData2 *data = get_file_data(addr);
    if (send(sock, data, sizeof(FileData2), 0) < 0)
    {
        perror("not sent\n");
        return -1;
    }
    free(data);
    return 0;
}

void handle_client_rw(int client_socket)
{

    printf("Client connected\n");

    Message msg;
    if (recv(client_socket, &msg, sizeof(Message), 0) < 0)
    {
        perror("I'm sleepy");
    }

    int lock_return_value;
    int index = search(msg.path);
    printf("|| Path: %s, Index: %d\n",msg.path, index);
    // !The below line is a big no no. It changes the location of the lock for each thread. That was causing the errors 
    // FileData fdata = file_datas[index];
    printf("Command: %d\n", msg.command);
    if (msg.command == CMD_READ || msg.command == CMD_STREAM)
    {
        lock_return_value = pthread_rwlock_rdlock(&file_datas[index].lock);
        printf("read lock_return value %d\n", lock_return_value);
        if (lock_return_value != 0)
        {
            perror("read locked");
        }
        send_file(client_socket, rename_path2(msg.path));
    }
    else if (msg.command == CMD_WRITE)
    {
        lock_return_value = pthread_rwlock_wrlock(&file_datas[index].lock);
        printf("Pointer: %p\n", &file_datas[index].lock);
        printf("write lock_return value %d\n", lock_return_value);    
        if (lock_return_value != 0)
        {
            perror("write locked");
        }
        send_file(client_socket, rename_path2(msg.path));
        receive_file(client_socket, rename_path2(msg.path));
    }
    else if (msg.command == CMD_GET_INFO)
    {
        send_file_data(client_socket, rename_path2(msg.path));
    }
    else if(msg.command==CMD_COPY_RECV)
    {
        // for file case.
        // for this we have to append the part of the string after the last slash in the source path 
        //thats in the data field of the message to the destination path in the path field of the message.
        char* destnpath = (char*)calloc(MAX_PATH,sizeof(char));
        char* slashpoint = strrchr(msg.path,'/');
        strcpy(destnpath,".");
        strcat(destnpath,msg.data);
        strcat(destnpath,slashpoint);
        printf(" at handle_rw destnpath is : %s\n",destnpath);  
        if(msg.flags==0)
        {

            receive_file(client_socket,destnpath);
            Response copyreck;
            copyreck.status==STATUS_DONE;
            send(client_socket,&copyreck,sizeof(Response),0);
            close(client_socket);
            pthread_exit(NULL);
        }
        else
        {
            char unpack_command[256];
            char* dot =(char*)calloc(MAX_PATH_LENGTH,sizeof(char));
            char* parent_folder = strdup(msg.data);
            //append . to the beginning of parent_folder
            parent_folder = strcat(dot,parent_folder);    
            // Create the directory structure
            char mkdir_command[256];
            snprintf(mkdir_command, sizeof(mkdir_command), "mkdir -p temp%s", parent_folder);
            system(mkdir_command);
            snprintf(unpack_command, 1000,
            "tar -xzf temp.tar.gz -C ./temp");
            // snprintf(unpack_command, sizeof(unpack_command),
            // "tar -xzf temp.tar.gz -C \".%s\"", parent_folder);
            
            printf(" at handle_rw unpack_command is : %s\n",unpack_command);
            
            receive_file(client_socket,rename_path2(("/temp.tar.gz")));

            system(unpack_command);
            remove("./temp.tar.gz");
            char move_command[256];

            struct stat st;
            if (stat(destnpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                snprintf(move_command, sizeof(move_command), "mv temp/* %s", dirname(destnpath));
            } else {
                snprintf(move_command, sizeof(move_command), "mv temp/* %s", dirname(destnpath));
            }
            system(move_command);
            // Remove the temp folder
            system("rm -rf temp.tar.gz");
            system("rm -rf temp");

            Response copy_ack;
            copy_ack.status = STATUS_DONE;
            if (send(client_socket, &copy_ack, sizeof(Response), 0) < 0)
            {
                perror("copy ack not sent\n");
            }
            close(client_socket);
            pthread_exit(NULL);

        }
        
    }

    Response ack;
    int unlock;
    if (recv(client_socket, &ack, sizeof(Response), 0) < 0)
    {
        perror("did not successfully finish reading/writing/streaming");
    }
    if (ack.status != STATUS_DONE) {
        perror("did not succesfully read/write/stream, informed by client"); 
    }
    if (lock_return_value == 0) {
        // if locking was successful
        unlock = pthread_rwlock_unlock(&file_datas[index].lock);
        if(unlock != 0)
        {
            perror("unlock:");
            exit(1);
        }
    } else {
        printf("Either naming server has incorrectly infomred the client, or the file was already locked somehow, or system error\nlock_return_data: %d\n", lock_return_value); 
    }
    
    // Close the client socket
    close(client_socket);

    // Exit the thread
    pthread_exit(NULL);
}


// if to_write is zero, checks if reading is possible. Else, checks if writing is possible
// @return 0 if not possible, 1 if possible, -1 on error
int is_accessible_rw(int to_write, char *path)
{
    printf(" entering is_accessible_rw, %d\n", to_write);
    int result;
    int index = search(path); 
    printf("index of %s in array: %d", path, index);

    if (to_write)
    {
        printf("Checking for write\n");
        result = pthread_rwlock_trywrlock(&(file_datas[index].lock));
    }
    else
    {
        result = pthread_rwlock_tryrdlock(&(file_datas[index].lock));
    }
    printf("(16 if busy) result: %d\n", result);
    if (result == 0)
    {
        // Lock was acquired successfully
        pthread_rwlock_unlock(&(file_datas[index].lock));
        return 1;
    }
    else if (result == EBUSY)
    {
        // Lock is held by another thread
        return 0;
    }
    else
    {
        // Other error occurred
        return -1;
    }
}

int connect_to_sscopy(char *dest_ip, int dest_port)
{
    // Create a socket
    int destsock = socket(AF_INET, SOCK_STREAM, 0);
    if (destsock < 0)
    {
        perror("destsocket");
        return -1;
    }

    // Connect to the naming server
    struct sockaddr_in nm_addr;
    memset(&nm_addr, 0, sizeof(struct sockaddr_in));
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &nm_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    if (connect(destsock, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        perror("connect");
        return -1;
    }
    printf("Connected to storage server at %s:%d\n", ss_ip, ss_port);

    return destsock;
}




// void *handle_request(void *args)
// {
//     int sock = *((int *)args);
//     while (1)
//     {
//         Message msg;
//         ssize_t bytes_received = recv(sock, &msg, sizeof(Message), 0);
//         if (bytes_received < 0)
//         {
//             perror("recv from naming server failed");
//             close(sock);
//             // return -1; not a void* I'm replacing it with NULL
//             return NULL;
//         }
//         printf("HIIIIIIIIIII SDHASKDSAHDKASDJH %d, %s\n", msg.command, msg.path);
//         printf("does it enter the condition: %d\n",msg.command == CMD_READ || msg.command == CMD_STREAM || msg.command == CMD_WRITE);
//         if (msg.command == CMD_READ || msg.command == CMD_STREAM || msg.command == CMD_WRITE)
//         {
//             printf("JKJJJJJJJJ Received command type: SDDDDDDDDD \n");
//             // try to establish a connection
//             // message sent back tells if possible or not
//             Response rsp;
//             // TODO: assuming success currently. change with write lock
//             if (is_accessible_rw(msg.command == CMD_WRITE, msg.path))
//             {
//                 rsp.status = STATUS_SUCCESS;
//             }
//             else
//             {
//                 rsp.status = STATUS_ERR_WRITE_CONFLICT;
//             }

//             // Send information to naming server
//             if (send(sock, &rsp, sizeof(Response), 0) < 0)
//             {
//                 perror("Fucked something up");
//                 pthread_exit(NULL);
//             }

//             // wait for response from client only if possible to perform action
//             if (rsp.status == STATUS_SUCCESS)
//             {
//                 pthread_t tid_client_handler;
//                 pthread_create(&tid_client_handler, NULL, wait_for_client_connection_rw, NULL);

//                 pthread_detach(tid_client_handler);
//             }
//         }
//         else if (msg.command == CMD_COPY || msg.command == CMD_CREATE || msg.command == CMD_DELETE)
//         {
//             // just do it, and tell the naming server that it's happpened (or not).
//         }
//     }
//     pthread_exit(NULL);
// }

void* listener_thread(void* arg) {
    int ss_port2 = *(int*)arg;
    
    // Create socket
    int sock_listener = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listener < 0) {
        perror("socket creation failed");
        return NULL;
    }

    // Allow socket reuse
    int opt = 1;
    if (setsockopt(sock_listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sock_listener);
        return NULL;
    }

    // Prepare socket address structure
    struct sockaddr_in addr_listener;
    memset(&addr_listener, 0, sizeof(addr_listener));
    addr_listener.sin_family = AF_INET;
    addr_listener.sin_port = htons(ss_port2);
    addr_listener.sin_addr.s_addr = inet_addr(ss_ip);  // Using global ss_ip

    // Bind socket
    if (bind(sock_listener, (struct sockaddr*)&addr_listener, sizeof(addr_listener)) < 0) {
        perror("bind failed");
        close(sock_listener);
        return NULL;
    }

    // Listen for connections
    if (listen(sock_listener, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(sock_listener);
        return NULL;
    }

    printf("Listening on port %d\n", ss_port2);

    // Continuous listening loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accept incoming connection
        int client_socket = accept(sock_listener, 
                                   (struct sockaddr*)&client_addr, 
                                   &client_len);
        
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        // Handle the connection 
        // You'll need to implement the connection handling logic here
        // This might involve creating another thread to handle the client
        // or processing the request directly
        printf("Received connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));

        // Example: Close connection immediately (replace with actual handling)
        close(client_socket);
    }

    // Close listener socket (though this will never be reached in this example)
    close(sock_listener);
    return NULL;
}


int main(int argc, char *argv[])
{
    printf("gets here\n");
    /*
    Steps:
    args: <nm_ip> <nm_port> <ss_ip> <ss_port>
    1. Create a socket to connect to the name server
    2. Connect to the name server
    3. Register with the name server
    4. Create a socket to listen for incoming connections from clients
    5. Create a thread to handle incoming connections from clients
    */

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <nm_ip> <nm_port> <ss_ip> <ss_port>\n", argv[0]);
        exit(1);
    }
    char *nm_ip = argv[1];
    int nm_port = atoi(argv[2]);
    ss_ip = argv[3];
    ss_port = atoi(argv[4]);

    /*
    To register with the naming server, we need the following information
        1. IP address of the storage server
        2. Port number of the storage server
        3. List of files stored in the storage server
    */

    // Naming Server part goes here
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    // Connect to the naming server
    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(nm_port);
    if (inet_pton(AF_INET, nm_ip, &nm_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        perror("connect");
        return -1;
    }
    printf("Connected to naming server at %s:%d\n", nm_ip, nm_port);

    if (sock < 0)
    {
        perror("couldn't connect to naming server\n");
        return -1;
    }

    // im gonna open the socket that the server will be listening to for client requests.
    int opt =1;
    int ss_socket = socket(AF_INET, SOCK_STREAM , 0);
    if (ss_socket < 0)
    {
        perror("socket");
        exit(1);
    }
    int sockopterr = setsockopt(ss_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(ss_socket));
    if (sockopterr == -1)
    {
        perror("socket option:");
        exit(1);
    }
    struct sockaddr_in ss_addr;
    memset(&ss_addr, 0, sizeof(struct sockaddr_in));
    ss_addr.sin_family = AF_INET;
    ss_addr.sin_port = htons(ss_port);
    ss_addr.sin_addr.s_addr = inet_addr(ss_ip);
    // printf("wait hereeeeeeeeeeeee11111 \n");
    if (bind(ss_socket, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
    if (listen(ss_socket, MAX_CLIENTS) < 0)
    {
        perror("listen");
        exit(1);
    }
    printf("Storage Server is listening on %s:%d\n", ss_ip, ss_port);
    
    
    // Send information to naming server
    if (send(sock, "S", 1, 0) < 0)
    {
        perror("send");
        return -1;
    }

    //     SSData ss;
    //     ss.ip = ss_ip;
    //     ss.port = ss_port;
    //     get_directory_structure(&ss);

    StorageServer ss;
    strcpy(ss.ss_ip, ss_ip);
    ss.client_port = ss_port;
    ss.nm_port = nm_port;
    ss.Backup = NULL;
    ss.I_ded = 0; 
    ss.I_ded = 0; 
    opt = 1;
    
    // SSData ss;
    // ss.ip = ss_ip;
    // ss.port = ss_port;
    search_files();
    for (int i = 0; i < len_files; i++)
    {
        if (strcmp(files[i], "") == 0) {
            // Replace the empty string with a "/"
            files[i] = strdup("/");
            // Replace file_datas[i].path with a "/"
            file_datas[i].path = strdup("/");
        }

        printf("File: %s\n", files[i]);
        printf("%s, %d, %d\n", file_datas[i].path, *(&file_datas[i].lock), file_datas[i].type);
    }

    // ss.dir_len = len_files;
    // ss.files = files;

    if (send(sock, &ss, sizeof(StorageServer), 0) < 0)
    {
        perror("send");
        return -1;
    }
    else
    {
        printf("Sent intial data to Naming Server: SS");
    }
    if (send(sock, &ss, sizeof(StorageServer), 0) < 0)
    {
        perror("send");
        return -1;
    }
    else
    {
        printf("Sent intial data to Naming Server: BS");
    }

    if (send(sock, &len_files, sizeof(int), 0) < 0)
    {
        perror("send");
        return -1;
    }
    else
    {
        printf("Sent number of files to Naming Server");
    }

    for (int i = 0; i < len_files; i++)
    {
        printf("Sending file directory %d\n", i);
        int path_length = strlen(files[i]);
        if (send(sock, &path_length, sizeof(int), 0) < 0)
        // if (send(sock, files[i], strlen(files[i])+1, 0) < 0)
        {
            perror("send");
            return -1;
        }
        if (send(sock, files[i], path_length + 1, 0) < 0)
        // if (send(sock, files[i], strlen(files[i])+1, 0) < 0)
        {
            perror("send");
            return -1;
        }
        else
        {
            printf("Sent file directory %d\n", i);
        }
    }

    int ssid; 
    if (recv(sock, &ssid, sizeof(int), NULL) < 0) {
        perror ("Couldn't receive SSID"); 
    }

    printf("ssid: %d\n", ssid); 

    int ss_port2 = ssid + 6000; 
    // printf("port being bound to: %d\n", ss_port2);
    // int sock_listener = socket(AF_INET, SOCK_STREAM, 0);
    // struct sockaddr_in addr_listener;
    // memset(&addr_listener, 0, sizeof(addr_listener));
    // addr_listener.sin_family = AF_INET;
    // addr_listener.sin_port = htons(ss_port2);
    // addr_listener.sin_addr.s_addr = INADDR_ANY; // Or specific IP

    // if (bind(sock_listener, (struct sockaddr*)&addr_listener, sizeof(addr_listener)) < 0) {
    //     perror("bind failed");
    //     exit(EXIT_FAILURE);
    // }

    // pthread_t listener_thread_id;
    // pthread_create(&listener_thread_id, NULL, listener_thread, &ss_port2);

    // Get message from naming server,to respond as to whether the operation is feasible or not..
    while (1)
    {
        // receiving the actual request sent from  from the naming server.
        //(which is the forwarded request from the client).
        Message msg;
        ssize_t bytes_received = recv(sock, &msg, sizeof(Message), 0);
        if (bytes_received < 0)
        {
            perror("recv from naming server failed");
            close(sock);
            return -1;
        }

        printf("Received create: %d\n", (msg.command == CMD_CREATE));
        printf("Recevied command: %d\n", msg.command);

        if (msg.command == CMD_READ || msg.command == CMD_STREAM || msg.command == CMD_WRITE || msg.command == CMD_GET_INFO)
        {
            // so the reponse we send back is to the naming server, not the client,
            // to inform whether the requested operation is possible or not.
            Response rsp;
            int index = search(msg.path);
            printf("Path: %s\nIndex: %d\n",msg.path,index);
            printf("Index: %d\n", index);
            if(index==-1)
            {
                printf("No such file\n");
                rsp.status = STATUS_ERR_FILE_NOT_FOUND;
            }
            else if (file_datas[index].type == 1) {
                // if directory
                printf("Entering here for directory\n");
                rsp.status = STATUS_ERR_FILE_NOT_FOUND;
            }
            // TODO: assuming success currently. change with write lock
            else if (is_accessible_rw(msg.command == CMD_WRITE, msg.path))
            {
                printf("says file is accessible\n");
                rsp.status = STATUS_SUCCESS;
            }
            else
            {
                printf("says file is not accessible\n");
                rsp.status = STATUS_ERR_WRITE_CONFLICT;
            }
            if (send(sock, &rsp, sizeof(Response), 0) < 0)
            {
                perror("Fucked something up");
            }

            // Create a socket to listen for incoming connections from a client
            
            pthread_t tid_client_handler;
            pthread_create(&tid_client_handler, NULL, wait_for_client_connection_rw, &ss_socket);

            pthread_detach(tid_client_handler);
        }
        else if (msg.command == CMD_CREATE) {
            // Assumption - 0 for file, 1 for directory
            int type = msg.flags;
            char *path = msg.path;
            char *name = msg.data;

            printf("Creating %s %s\n", type == 0 ? "file" : "directory", name);
            printf("Path: %s\n", path);

            char *new_path = malloc(strlen(path) + strlen(name) + 10);
            
            strcpy(new_path, ".");
            strcat(new_path, path);
            // Check if the last character is a '/'
            if (new_path[strlen(new_path) - 1] != '/')
                strcat(new_path, "/");
            strcat(new_path, name);

            printf("Trying to create %s\n", new_path);

            if (type == 0) {
                FILE *file = fopen(new_path, "w");
                if (file == NULL) {
                    perror("fopen");
                    return -1;
                }
                fclose(file);
            } else if (type == 1) {
                int status = mkdir(new_path, 0777);
                if (status == -1) {
                    perror("mkdir");
                    return -1;
                } 
            }

            Response rsp;
            rsp.status = STATUS_SUCCESS;
            if (send(sock, &rsp, sizeof(Response), 0) < 0)
            {
                perror("Fucked something up");
            }
            // updating FILES and FILE_DATAS
            int idx = len_files++; 
            files[idx] = calloc(strlen(new_path)+1,sizeof(char)); 
            file_datas[idx].path = calloc(strlen(new_path)+1,sizeof(char)); 
            strcpy(files[idx], new_path+1);
            strcpy(file_datas[idx].path, new_path+1);
            file_datas[idx].type = type; 
            file_datas[idx].metadata = NULL; 
            // lock
            if (pthread_rwlock_init(&(file_datas[idx].lock), NULL) != 0)
            {
                perror("could not initialise rwlock");
            }
            
        }
        else if (msg.command == CMD_DELETE)
        {
            char *path = msg.path;
            char *name = msg.data;

            if (strcmp(name, "/") == 0) {
                Response rsp;
                rsp.status = STATUS_ERR_FILE_NOT_FOUND;
                if (send(sock, &rsp, sizeof(Response), 0) < 0)
                {
                    perror("Fucked something up");
                }
                continue;
            }


            char *new_path = malloc(strlen(path) + strlen(name) + 10);
            strcpy(new_path, path);
            if (new_path[strlen(new_path) - 1] != '/')
                strcat(new_path, "/");
            strcat(new_path, name);

            // Searching if `new_path` is a substring of any of the files
            int found = 0;
            for (int i = 0; i < len_files; i++) {
                if (strstr(files[i], new_path) != NULL) {
                    found++;
                }
            }

            if (found == 0) {
                Response rsp;
                rsp.status = STATUS_ERR_FILE_NOT_FOUND;
                if (send(sock, &rsp, sizeof(Response), 0) < 0)
                {
                    perror("Fucked something up");
                }
                continue;
            }

            char* path_to_delete = malloc(strlen(new_path) + 10);
            strcpy(path_to_delete, ".");
            strcat(path_to_delete, new_path);
            printf("Path to delete: %s\n",path_to_delete);
            
            // Check if it is a file or a directory
            struct stat sb;
            if (stat(path_to_delete, &sb) == 0 && S_ISDIR(sb.st_mode)) {
                // Directory
                char command[100];
                sprintf(command, "rm -r %s", path_to_delete);
                int status = system(command);
                printf("status: %d\n", status);
                if (status == -1) {
                    Response rsp;
                    rsp.status = STATUS_ERR_FILE_NOT_FOUND;
                    if (send(sock, &rsp, sizeof(Response), 0) < 0)
                    {
                        perror("Fucked something up");
                    }
                    continue;
                }
            } else {
                // File
                if(is_accessible_rw(1,new_path))
                {
                    int status = remove(path_to_delete);
                    if (status == -1) {
                        Response rsp;
                        rsp.status = STATUS_ERR_FILE_NOT_FOUND;
                        if (send(sock, &rsp, sizeof(Response), 0) < 0)
                        {
                            perror("Fucked something up");
                        }
                        continue;
                    }
                }
                else
                {
                    Response rsp;
                    rsp.status = STATUS_ERR_WRITE_CONFLICT;
                    if(send(sock, &rsp, sizeof(Response), 0) < 0)
                    {
                        perror("Couldn't send:");
                    }
                    continue;
                }
            }

            // Update the files and file_datas.
            for (int i = 0; i < len_files; i++) {
                if (strstr(files[i], new_path) != NULL) {
                    free(files[i]);
                    free(file_datas[i].path);
                    for (int j = i; j < len_files - 1; j++) {
                        files[j] = files[j + 1];
                        file_datas[j] = file_datas[j + 1];
                    }
                    len_files--;
                    i--;
                }
            }

            Response rsp;
            rsp.status = STATUS_SUCCESS;
            if (send(sock, &rsp, sizeof(Response), 0) < 0)
            {
                perror("Fucked something up");
            }
        }
        else if (msg.command == CMD_COPY_SEND)
        {
            /// REMEMBER THAT IN THIS PART WE ONLY CHECK AS TO WHETHER WE CAN COPY OR NOT
            // AND THEN SEND A RESPONSE TO THE NAMING SERVER


            // msg.data contains the destination output parent path path,
            // so when you copy the file, you need to append the last part of the path to the destination path.

            // just do it, and tell the naming server that it's happpened (or not).
            // here message received is from naming server, so we check if we can send the file to the destination.
            int fileidx = search(msg.path);
            if(fileidx==-1)
            {
                // file not found
                Response resp;
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                send(sock,&resp,sizeof(Response),0);
                continue;
            }
            // this condition may need to be improved more.Just to check if we can read the file to copy it.
            if(is_accessible_rw(0,msg.path)==0)
            {
                // file is busy
                Response resp;
                resp.status = STATUS_ERR_WRITE_CONFLICT;
                send(sock,&resp,sizeof(Response),0);
                continue;
            }
            Response resp;
            resp.status = STATUS_SUCCESS;
            send(sock,&resp,sizeof(Response),0);
        
            if(strcmp(ss_ip,msg.dest_ip) == 0 && ss_port==msg.dest_port)
            {

                // For copy, message.path contains the file/folder to be copied
                // message.data contains the destination path
                printf("Source and destination server are the same.\n");
                char src[MAX_PATH];
                strcpy(src, ".");

                strcat(src, msg.path);

                char dest[MAX_PATH];
                strcpy(dest, ".");

                strcat(dest, msg.data);
                char command[MAX_PATH_LENGTH * 2 + 100];
                sprintf(command, "cp -r %s %s", src, dest);                
                if (system(command) == -1)
                {
                    perror("system");
                    resp;
                    resp.status = STATUS_ERR_FILE_NOT_FOUND;
                    send(sock,&resp,sizeof(Response),0);
                    continue;
                }

            }
            
            else
            {
                // now to establish a connection with the destination server.
                // and send the file to the destination server.
                //msg.dest_ip, msg.dest_port , contain the destination server information to connect to.
                printf("msg.dest_ip: %s\n",msg.dest_ip);
                printf("msg.dest_port: %d\n",msg.dest_port);
                int dest_sock = connect_to_sscopy(msg.dest_ip,msg.dest_port);
                if(dest_sock==-1)
                {
                    // error handling
                    perror("could not connect to destination server");
                    Response resp;
                    resp.status = STATUS_ERR_FILE_NOT_FOUND;
                    send(sock,&resp,sizeof(Response),0);
                    continue;
                }

                else
                {
                    // now we send the file to the destination server.
                    
                    char* destnpath = (char*)calloc(MAX_PATH,sizeof(char));
                    strcpy(destnpath,msg.data);
                    Message* destmsg2 = (Message*)calloc(1,sizeof(Message));
                    destmsg2->command = CMD_COPY_RECV;
                    strcpy(destmsg2->path,msg.path);
                    strcpy(destmsg2->data,destnpath);

                    struct stat path_stat;
                    stat(rename_path2(msg.path), &path_stat);
                    if (S_ISREG(path_stat.st_mode)) {
                        // It's a file
                        destmsg2->flags = 0;
                        send(dest_sock,destmsg2,sizeof(Message),0);
                        send_file(dest_sock,rename_path2(msg.path));
                        Response dest_resp; 
                        if(recv(dest_sock,&dest_resp,sizeof(Response),0)<0)
                        {
                            perror("could not receive response from destination server");
                            Response resp;
                            resp.status = STATUS_ERR_FILE_NOT_FOUND;
                            send(sock,&resp,sizeof(Response),0);
                            continue;
                        }
                        close(dest_sock);
                        continue;
                    } 
                    else if (S_ISDIR(path_stat.st_mode)) 
                        {
                        // It's a directory
                        // Handle directory case
                        destmsg2->flags = 1;
                        //we send the file as a zip instead., so we create a zip file, using exec
                        // the name of the file will be destmsg2.data with .zip appended
                        char* tar_command = (char*)calloc(2000,sizeof(char));
                        char *path_copy1 = strdup(msg.path);
                        char *path_copy2 = strdup(msg.path);
                        
                        //tar command
                        // printf(".%s\n",dirname(path_copy1));
                        // printf(".%s\n",basename(path_copy2));
                        char* buf= (char*)calloc(4098,sizeof(char));
                        getcwd(buf,4096);
                        snprintf(tar_command, 2000,
                            "tar -czf temp.tar.gz --transform='s,^.*/,,g' -C \".%s\" \"%s\"",
                            dirname(path_copy1), basename(path_copy2));
                        
                        printf("tar_command: %s\n",tar_command);
                        system(tar_command);
                        //int res = system(zipcommand); 
    
                        send(dest_sock,destmsg2,sizeof(Message),0);
                        
                        send_file(dest_sock,rename_path2(("/temp.tar.gz")));
                        //send_file(dest_sock,"./temp.zip");
                        // now we wait for the response from the destination server.
                        Response dest_resp;
                        if(recv(dest_sock,&dest_resp,sizeof(Response),0)<0)
                        {
                            perror("could not receive response from destination server");
                            Response resp;
                            resp.status = STATUS_ERR_FILE_NOT_FOUND;
                            send(sock,&resp,sizeof(Response),0);
                            continue;
                        }
                        close(dest_sock);
                        continue;

                    }
                    
                }
            }
            
        }
        else if(msg.command==CMD_COPY_RECV)
        {
            // searching if the source file exists in the destination server.
            // this condition may need to be improved more.
            printf("entered copy receive\n");
            printf("msg.path: %s\n",msg.path);
            printf("msg.data: %s\n",msg.data);
            // here just append a / to the path in the data field of the message.
            char* destnpath = (char*)calloc(MAX_PATH,sizeof(char));
            strcpy(destnpath,msg.data);
            if(destnpath[strlen(destnpath)-1]!='/')
            {
                strcat(destnpath,"/");
            }
            printf("destnpath %s\n",destnpath);
            int fileidx = search(msg.data);
            if(fileidx==-1)
            {
                // file not found
                Response resp;
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                send(sock,&resp,sizeof(Response),0);
                continue;
            }
            // telling the naming server that it is possible
            printf("msg.path %s\n",msg.path);
            int srcfileidx = search(msg.path);
            if(srcfileidx!=-1)
            {
                printf("Destination server and source server are the same.\n");
                // Send a success response.
                Response resp;
                resp.status = STATUS_SUCCESS;
                send(sock,&resp,sizeof(Response),0);
                // Copying the file logic is handled in the CMD_COPY_SEND part.
                continue;
            }
            Response resp;
            resp.status = STATUS_SUCCESS;
            send(sock,&resp,sizeof(Response),0);

            pthread_t tid_client_handler;
            pthread_create(&tid_client_handler, NULL, wait_for_client_connection_rw, &ss_socket);

            // Wait for the thread to finish
            pthread_join(tid_client_handler, NULL);
        
            // Update the files and file_datas.
            search_files();

            for (int i=0; i<len_files; i++) {
                if (strcmp(files[i], "") == 0) {
                    // Replace the empty string with a "/"
                    files[i] = strdup("/");
                    // Replace file_datas[i].path with a "/"
                    file_datas[i].path = strdup("/");
                }
            }
        
        }
    }

    return 0;
}