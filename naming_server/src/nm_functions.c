#include"naming_server.h"
#include"ss_registry.h"
#include"nm_bookkeeping.h"
#include"trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "cache.h"
#include <inttypes.h>
#include<libgen.h>

// setting I_ded
// source: https://stackoverflow.com/a/61960339/9873902 
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
// #include <linux/time.h>

// @return -1 on failure, 1 if timeout, success on 0
int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, unsigned int timeout_ms)
{
    int rc = 0; 
    // Set O_NONBLOCK
    int sockfd_flags_before;
    if((sockfd_flags_before=fcntl(sockfd,F_GETFL,0)<0)) return -1;
    if(fcntl(sockfd,F_SETFL,sockfd_flags_before | O_NONBLOCK)<0) return -1;
    // Start connecting (asynchronously)
    do {
        if (connect(sockfd, addr, addrlen)<0) {
            // Did connect return an error? If so, we'll fail.
            if ((errno != EWOULDBLOCK) && (errno != EINPROGRESS)) {
                rc = -1;
            }
            // Otherwise, we'll wait for it to complete.
            else {
                // Set a deadline timestamp 'timeout' ms from now (needed b/c poll can be interrupted)
                struct timespec now;
                if(clock_gettime(CLOCK_MONOTONIC, &now)<0) { rc=-1; break; }
                struct timespec deadline = { .tv_sec = now.tv_sec,
                                             .tv_nsec = now.tv_nsec + timeout_ms*1000000l};
                // Wait for the connection to complete.
                do {
                    // Calculate how long until the deadline
                    if(clock_gettime(CLOCK_MONOTONIC, &now)<0) { rc=-1; break; }
                    int ms_until_deadline = (int)(  (deadline.tv_sec  - now.tv_sec)*1000l
                                                  + (deadline.tv_nsec - now.tv_nsec)/1000000l);
                    if(ms_until_deadline<0) { rc=0; break; }
                    // Wait for connect to complete (or for the timeout deadline)
                    struct pollfd pfds[] = { { .fd = sockfd, .events = POLLOUT } };
                    rc = poll(pfds, 1, ms_until_deadline);
                    // If poll 'succeeded', make sure it *really* succeeded
                    if(rc>0) {
                        int error = 0; socklen_t len = sizeof(error);
                        int retval = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
                        if(retval==0) errno = error;
                        if(error!=0) rc=-1;
                    }
                }
                // If poll was interrupted, try again.
                while(rc==-1 && errno==EINTR);
                // Did poll timeout? If so, fail.
                if(rc==0) {
                    errno = ETIMEDOUT;
                    rc=-1;
                }
            }
        }
    } while(0);
    // Restore original O_NONBLOCK state
    if(fcntl(sockfd,F_SETFL,sockfd_flags_before)<0) return -1;
    // Success
    return rc;
}


// wrapper
int storage_server_is_dead(unsigned int timeout_ms, int ssid) {
    StorageServer *ss = myssl.Storage_servers[ssid]; 
    // printf("Accessed ssip: %d\n", ss->client_port); 
    int ss_port2 = 6000 + ssid; 
    // printf("second port: %d\n", ss_port2);
    // // Create socket
    // int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (sockfd < 0) return -1;

    // // Prepare sockaddr_in structure for storage server
    // struct sockaddr_in ss_addr;
    // memset(&ss_addr, 0, sizeof(ss_addr));
    // ss_addr.sin_family = AF_INET;
    // ss_addr.sin_port = htons(ss_port2);
    
    // // Convert IP to network address
    // if (inet_pton(AF_INET, ss->ss_ip, &ss_addr.sin_addr) <= 0) {
    //     close(sockfd);
    //     return -1;
    // }

    // // Call connect_with_timeout 
    // int result = connect_with_timeout(sockfd, 
    //                                   (struct sockaddr*)&ss_addr, 
    //                                   sizeof(ss_addr), 
    //                                   timeout_ms);

    // // Close socket
    // close(sockfd);

    // // Interpret result
    // if (result == -1 && errno != ETIMEDOUT && errno != ECONNREFUSED)
    // {
    //     perror("connect_with_timeout");
    //     return -1;
    // }
    // else if (result == -1 && (errno == ETIMEDOUT || errno == ECONNREFUSED))
    // {
    //     return 1;
    // }

    // return 0;  // success

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ss_port2);
    inet_pton(AF_INET, ss->ss_ip, &addr.sin_addr);

    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);

    if (result < 0) {
        // Connection failed
        log_nm_operation("Storage server is down");
        return 1;
    }
    return 0;
}

//TODO: Define this trie somewhere. I'm just gonna use it a bunch without making it - Rijul

void initialize_ss_registration()
{

}

// send a message to the storage server.
 //@return socket_filedesc if the file was created successfully, -1 if the file was not created successfully.
int send_msg(StorageServer*ss, Message* msg)
{
    int sockfd;
    struct sockaddr_in server_addr;
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }
    
    
    memset(&server_addr, 0, sizeof(server_addr));
    // Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ss->nm_port);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, ss->ss_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        return -1;
    }

    // Connect to the storage server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }
    // now we send the request to create the file to the storage server.
    if(send(sockfd, msg, sizeof(Message), 0)<0)
    {
        close(sockfd);
        return -1;
        //error handling
    }
    // printf("sent message\n");
    //message sent fine
    return sockfd;
}

int send_copy_msg(StorageServer* ss, CopyMessage* cpmsg)
{
    if((send(ss->sock_fd,cpmsg,sizeof(CopyMessage),0))<0)
    {
        log_nm_operation("Send Copy Message Failed.");
        return -1;
    }
    // Set up server address
    return 0;
}

uint64_t find_file_storage_system_index(char * file_name)
{
    uint64_t index;
    index = find_in_cache(file_name);
    if(index!=UINT64_MAX)
        return index;
    
    void *data = trie_search(files,file_name);
    if (data == NULL) {
        //Do something implying that file isn't real but for now
        return UINT64_MAX;
    }
    index = (uint64_t) data - 1; // this should be the correct index if the pointer conversion worked as intended
    add_to_cache(file_name,index);
    return index;
}


int 
get_directory_structure(struct trie *self, const char *key)
{
    int len = strlen(key);
    char *folder_path = calloc(len+3,sizeof(char));
    strcpy(folder_path,key);
    folder_path[len]='/';
    die = malloc(sizeof(struct chopping_block));
    die->index=0;
    size_t num_files = trie_count(self,folder_path);
    die->file_names=calloc(num_files,sizeof(char*));
    if(trie_visit(self,folder_path,add_to_chopping_block,NULL)!=0)
    {
        log_nm_operation("Chopping block: found one.");
    }
    return 0;
}

StorageServer *find_file_location(char *file_name)
{
    uint64_t index = find_file_storage_system_index(file_name);
    

    if(index == UINT64_MAX) //file doesn't exist
        return NULL; 
    
    // check if alive
    myssl.Storage_servers[index]->I_ded = storage_server_is_dead(1000, index);
    if (myssl.Storage_servers[index]->I_ded) {
        char messge[100];
        sprintf(messge, "Storage server is down. Attempting to access backup at port %d\n", myssl.Storage_servers[index]->Backup->client_port);
        log_nm_operation(messge);

    } else {
        char messge[100];
        sprintf(messge, "Storage server is live. Attempting to connect server at port %d\n", myssl.Storage_servers[index]->client_port); 
        log_nm_operation(messge);
    
    }
    
    if(myssl.Storage_servers[index]->I_ded==0) //server is up 
    {
        return myssl.Storage_servers[index];
    }
    else
        return myssl.Storage_servers[index]->Backup;
}




//returns 0 if the file was created successfully, -1 if the file was not created successfully.
int handle_create(StorageServer* ss, Message* msg)
{
    //first we're checking if the file already exists.
    // the files true name would be the concatenation of the path and the data.
    char* absolutepath =(char*)calloc(strlen(msg->path)+strlen(msg->data)+2,sizeof(char));
    strcpy(absolutepath,msg->path);
    if (msg->path[strlen(msg->path) - 1] != '/')
        strcat(absolutepath,"/");
    strcat(absolutepath,msg->data);
    StorageServer* loc = find_file_location(absolutepath);
    if(loc!=NULL)
    {
        //file already exists.
        return -1;
    }
   if((send(ss->sock_fd, msg,sizeof(Message),0))<0)
    {
        log_nm_operation("Send failed (create).");
    }
    Response resp;
    if(recv(ss->sock_fd, &resp, sizeof(Response), 0)<0)
    {
        // storage server didn't respond.
        log_nm_operation("Storage server did not respond");
        return -1;
        //error handling#
    }
    //if all went fine, we can send the response to the client.
    //ADD CHECKING FOR THE RESPONSE STATUS HERE.
    //just close the socket and return the status.
    return 0;
}

//returns 0 if the file was deleted successfully, -1 if the file was not deleted successfully, -2 if the file is currently in use
int handle_delete(StorageServer* ss, Message* msg)
{
    //we're just forwarding the request to the storage server.
    // we can now receive the response from the storage server.
    if((send(ss->sock_fd,msg,sizeof(Message),0))<0)
    {
        log_nm_operation("Send failed (delete).");
    }
    Response resp;
    if(recv(ss->sock_fd, &resp, sizeof(Response), 0)<0)
    {
        //close(sockfd);
        // storage server didn't respond.
        log_nm_operation("Storage server did not respond");
        return -1;
        //error handling#
    }
    if(resp.status=STATUS_ERR_WRITE_CONFLICT)
    {
        return -2;
    }
    //if all went fine, we can send the response to the client.
    //ADD CHECKING FOR THE RESPONSE STATUS HERE.
    //just close the socket and return the status.
    //close(sockfd);
    return 0;
}

// i'm just chaning this to return the index of the storage server, in the storage server list.
// @return -1 if there'n no more space, else the index of the storage server in the storage server list.
int add_storage_server(StorageServer ss)
{
    // There's no current implementation to dealing with losing servers. I'm assuming that we're going to set them to NULL.
    //Also, I defined Storage_server_list in ss_registry.h. Makes sense for it to be there but feel free to move it.
    if(myssl.num_servers==myssl.max_servers)
    {
        return -1;
    }

    int i;
    for(i=0;i<myssl.max_servers;i++)
    {
        // find an unused spot in the array
        if(myssl.Storage_servers[i]==NULL)
        {
            // initialize Server
            myssl.Storage_servers[i] = malloc(sizeof(StorageServer));
            strcpy(myssl.Storage_servers[i]->ss_ip,ss.ss_ip);
            myssl.Storage_servers[i]->nm_port = ss.nm_port;
            myssl.Storage_servers[i]->client_port = ss.client_port;
            myssl.Storage_servers[i]->I_ded = ss.I_ded;
            // initialize backup
            if (ss.Backup->client_port != ss.client_port)
            {
                myssl.Storage_servers[i]->Backup = malloc(sizeof(StorageServer));
                myssl.Storage_servers[i]->Backup->nm_port = ss.Backup->nm_port;
                strcpy(myssl.Storage_servers[i]->Backup->ss_ip,ss.Backup->ss_ip);
                myssl.Storage_servers[i]->Backup->client_port = ss.Backup->client_port;
                myssl.Storage_servers[i]->Backup->Backup = NULL;
                myssl.Storage_servers[i]->Backup->I_ded = 0;
            } 
            else 
            {
                myssl.Storage_servers[i]->Backup = NULL;
            }
            
            
            //increment number of servers
            myssl.num_servers++;
            break;
        }
    }
    return i;
}

void add_file(char * file_name, uint64_t ss_index)
{
    void *data = (void *) (ss_index + 1);
    int res = trie_insert(files, file_name, data); 
    if (res!=0)
        perror("trie insert:");
    //function wtakes in data as void *. We can get output by calling the key.
    // printf("%d\n", res);
}

int remove_file(char * file_name)
{
    remove_from_cache(file_name);
    int del_out = trie_delete(files, file_name);
    trie_print(files);
    return del_out;
}




void initialize_nm(const char* public_ip, int public_port)
{
    //initialize_cache();


    //initialize_ss_registration("storage_server_list.txt");

    int server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0)
    {
        // Initialization failuer.
        exit(-1);
    }

    // Set socket options to allow quick reuse of the port (important for server restarts)
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        //log_nm_operation("Error setting socket options"ad);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
     // Define the server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Zero out the structure
    server_addr.sin_family = AF_INET;             // Set address family to IPv4
    server_addr.sin_port = htons(public_port);    // Convert port to network byte order

    // Convert IP address from string to binary form and set it in the structure
    if (inet_pton(AF_INET, public_ip, &server_addr.sin_addr) <= 0) {
        //log_nm_operation("Invalid IP address");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified IP address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        //log_nm_operation("Error binding socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Mark the socket as passive, ready to accept incoming connections
    if (listen(server_socket, 10) < 0) { // Backlog of 10
        //log_nm_operation("Error listening on socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Log that the server has started listening for connections
    log_nm_operation("Naming Server initialized and listening for connections");

    // Loop to accept and handle incoming connections
    int iter = 0; 
    while (1) 
    {
        printf("waiting on iteration %d\n", iter); iter++; 
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        // Check for errors during connection acceptance
        if (new_socket < 0) {
            log_nm_operation("Error accepting connection");
            continue; // Continue to the next iteration to accept new connections
        }

        // Receive the connection type identifier ('S' for SS, 'C' for Client)
        char connection_type;
        if (recv(new_socket, &connection_type, sizeof(connection_type), 0) <= 0) {
            log_nm_operation("Error receiving connection type");
            close(new_socket);
            continue;
        }
        printf("connection type: %c\n", connection_type); 
        // Determine the type of connection and handle accordingly
        if (connection_type == 'S'){
            printf("connects to storage server\n");
            // Handle Storage Server connection in a new thread
            pthread_t ss_thread;
            int* ss_socket = malloc(sizeof(int));
            *ss_socket = new_socket;
            pthread_create(&ss_thread, NULL, (void*)handle_ss_registration, ss_socket);
            pthread_detach(ss_thread); // Detach thread to allow it to clean up after itself
        } else if (connection_type == 'C') {
            printf("connects to client\n");
            // Handle Client connection in a new thread
            pthread_t client_thread;
            int* client_socket = malloc(sizeof(int));
            *client_socket = new_socket;
            pthread_create(&client_thread, NULL, (void*)handle_client_request, client_socket);
            pthread_detach(client_thread); // Detach thread to allow it to clean up after itself
        } else {
            // Log an error if the connection type is unknown
            log_nm_operation("Unknown connection type received");
            close(new_socket); // Close the socket for invalid connections
        }
    }
    // Close the server socket (this code is unreachable, but added for completeness)
    close(server_socket);
}

void* handle_ss_registration(void* arg)
{
    int server_socket = *((int*)arg);
    free(arg); // Free the memory allocated for the argument

    // Receive the Storage Server details
    StorageServer ss;
    if (recv(server_socket, &ss, sizeof(StorageServer), 0) <= 0) {
        log_nm_operation("Error receiving Storage Server details");
        close(server_socket);
        pthread_exit(NULL);
    }
    StorageServer bs;
    if (recv(server_socket, &bs, sizeof(StorageServer), 0) <= 0) {
        log_nm_operation("Error receiving Storage Server details");
        close(server_socket);
        pthread_exit(NULL);
    }
    printf("Registration: %s\n, %d\n", ss.ss_ip,ss.client_port);
    printf("Registration (backup): %s\n, %d\n", bs.ss_ip,bs.client_port);
    ss.Backup = &bs; 
    printf("Registration (backup): %s\n, %d\n", ss.Backup->ss_ip, ss.Backup->client_port);
    int ssid = add_storage_server(ss);
    printf("port stored in ssid array: %d (at index %d)\n", myssl.Storage_servers[ssid]->client_port, ssid);
    // int ssid = 0;
    if(ssid==-1)
    {
        //error handling
        log_nm_operation("Error adding Storage Server");
        close(server_socket);
        pthread_exit(NULL);
    }   
    log_nm_operation("Storage Server registered successfully");
    // Send a response to the Storage Server
    // Response response;
    // response.status = STATUS_SUCCESS;
    // strcpy(response.message, "Storage Server registered successfully");
    // send(server_socket, &response, sizeof(Response), 0);
    // // this message received will contain the number of files.
    
    int num_files;
    if(recv(server_socket, &num_files, sizeof(int), 0)<=0)
    {
        log_nm_operation("Error receiving Storage Server details (num files)");
        close(server_socket);
        pthread_exit(NULL);
    }
    printf("%d\n", num_files);
    // now we will receive an array of strings, each string being a file path.
    char**filepaths = (char**)calloc(num_files+1,sizeof(char*));
    for(int i=0;i<num_files;i++)
    {
        int path_length = 0; 
        if(recv(server_socket, &path_length, sizeof(int), 0)<=0)
        // if(recv(server_socket, filepaths[i], strlen(filepaths[i])+1, 0)<=0)
        {
            log_nm_operation("Error receiving Storage Server details (path length)");
            close(server_socket);
            pthread_exit(NULL);
        }
        filepaths[i] = (char*)calloc(path_length + 2,sizeof(char)); 
        if(recv(server_socket, filepaths[i], path_length + 1, 0)<=0)
        // if(recv(server_socket, filepaths[i], strlen(filepaths[i])+1, 0)<=0)
        {
            log_nm_operation("Error receiving Storage Server details (path)");
            close(server_socket);
            pthread_exit(NULL);
        }
    }
    for(int i=0;i<num_files;i++)
    {
        printf("%s\n", filepaths[i]); 
        add_file(filepaths[i],ssid);
    }
    trie_print(files);
    myssl.Storage_servers[ssid]->sock_fd=server_socket;
    printf("Connection with storage server established on socket %d\n",server_socket);
    log_nm_operation("Connection with storage server established");


    if (myssl.Storage_servers[ssid]->Backup == NULL) {
        printf("\n\n\n\n{{{{%d\n%d\nbackup port\n}}}}\n\n\n\n", ss.Backup->client_port, ss.client_port); 
        // if ss and ss.Backup are the same, then we have a backup port
        int i; 
        for (i = 0; i < myssl.max_servers; i++)
        {
            if (myssl.Storage_servers[i] == NULL) {
                log_nm_operation("original server not connected for this backup server");
                printf("original server not connected for this backup server"); 
                break;
            }
            if (myssl.Storage_servers[i]->Backup != NULL) {
                if (
                    myssl.Storage_servers[i]->Backup->client_port == myssl.Storage_servers[ssid]->client_port
                ) {
                    // find the backup port and add socket
                    myssl.Storage_servers[i]->Backup->sock_fd = server_socket; 
                }
                break; 
            }
            // printf("%d, %d, %d\n", myssl.Storage_servers[i]->sock_fd, server_socket, myssl.Storage_servers[i]->Backup->sock_fd);
        }
        // printf("%d, %d, %d\n", myssl.Storage_servers[i]->sock_fd, server_socket, myssl.Storage_servers[i]->Backup->sock_fd);
        
    }

    if (send(server_socket, &ssid, sizeof(ssid), NULL) < 0) {
        perror ("Couldn't send ssid details"); 
    } 


    //close(server_socket);
    pthread_exit(NULL);
    // Add the Storage Server to the registry

    // Insert accessible paths into search structures
    // Assume SS sends number of paths followed by each path
    // commented code for cache handling.
    
    // Example implementation:
    // int num_paths;
    // recv(ss_socket, &num_paths, sizeof(int), 0);
    // for (int i = 0; i < num_paths; i++) {
    //     char path[MAX_PATH_LENGTH];
    //     recv(ss_socket, path, sizeof(path), 0);
    //     insert_path(path, ss.ss_ip, ss.client_port);
    //     update_cache(path, ss.ss_ip, ss.client_port);
    // }
    // Close the socket and exit the thread
}


int get_index (StorageServer* ss) {
    for (int i = 0; i < myssl.max_servers; i++) {
        if (myssl.Storage_servers[i] == ss) {
            return i;
        }
    }
    return -1;
}


void* handle_client_request(void* arg) {
    int client_socket = *((int*)arg);
    free(arg);
    
    // Receive client request message
    Message msg;
    if (recv(client_socket, &msg, sizeof(Message), 0) <= 0) {
        log_nm_operation("Failed to receive Client request");
        close(client_socket);
        return NULL;
    }
    
    // Process the Client request based on command type
    Response resp;
    char* filepath;
    // int sock_fd;
    Response ss_response;
    StorageServer* ss;
    StorageServer* destss;
    switch (msg.command) {
        case CMD_STREAM:
        case CMD_READ:
            // Implement READ handling
            filepath = msg.path;
            // printf("gets hereeeee\n");
            ss = find_file_location(filepath);
            // printf("is ss at %d dead? %d\n", ss->client_port, ss->I_ded); 

            if(ss != NULL)
            {           
                // check if storage server is up

                // we just forward the request to the storage server.
                if((send(ss->sock_fd,&msg,sizeof(msg),0))<0)
                {
                    printf("send failed\n");
                }

                // we wait for the storage server to send back the response.]
                if(recv(ss->sock_fd, &ss_response, sizeof(Response), 0)<0)
                {
                    perror("Receive ss response");
                    //error handling
                    resp.status = STATUS_ERR_FILE_NOT_FOUND;
                    strcpy(resp.message, "File could not be written to");
                    send(client_socket, &resp, sizeof(Response), 0);
                    break;
                }
                
                // // add handling for the response here.
                // if(recv(sock_fd, &ss_response, sizeof(Response), 0)<0)
                // {
                //     //error handling
                //     resp.status = STATUS_ERR_FILE_NOT_FOUND;
                //     strcpy(resp.message, "File could not be doung to be read to ?");
                //     printf("Error reading from storage server\n");
                //     send(client_socket, &resp, sizeof(Response), 0);
                //     break;
                // }
                //close(sock_fd);
                // now we just forward this response to the client.
                resp.status = ss_response.status;
                // strcpy(resp.message, ss_response.message);
                resp.ss_port = ss->client_port;
                strcpy(resp.ss_ip,ss->ss_ip);
                if(send(client_socket, &resp, sizeof(Response), 0) < 0)
                {
                    log_nm_operation("Error sending response to client");
                    printf("Error sending response to client\n");
                }
                break;
            }
            else //file not found
            {
                printf("File not found\n");
                log_nm_operation("File not found");
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message,"File not found\n");
                if(send(client_socket, &resp, sizeof(Response), 0) < 0)
                {
                    log_nm_operation("Error sending response to client");
                    printf("Error sending response to client\n");
                }
                break;
            }
        case CMD_WRITE:
            // Implement WRITE handling

            // The client will send the path of the file, the name of the file, and writing mode
            // the naming server will then communicate the fact that a client wants to write to this file 
            // on the storage server.
            //  If this is feasible, the storage server will send a response to the naming server,
            // which will then send a response to the client.
            // The response will the StorageServer struct, in which the client is to use the client_port
            // to connect to the storage server. In order to begin the write operation.
            // here the entire filepath should exist in msg.path
            // I'm going to make an assumption that for WRITE to a file, the file must exist in the NFS already
            filepath = msg.path;
            int write_mode = msg.flags;
            ss = find_file_location(filepath);
            if(ss == NULL)
            {
                printf("File not found\n");
                log_nm_operation("File not found");
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                if(send(client_socket, &resp, sizeof(Response), 0) < 0)
                {
                    log_nm_operation("Error sending response to client");
                    printf("Error sending response to client\n");
                }
                break;
            }
            else
            {
                // now we can send the request to the storage server.
                // we just forward the request to the storage server.
                if((send(ss->sock_fd,&msg,sizeof(msg),0))<0)
                {
                    printf("send failed\n");
                }
                // now we can receive the response from the storage server.
                if(recv(ss->sock_fd, &ss_response, sizeof(Response), 0)<0)
                {
                    //error handling
                    resp.status = STATUS_ERR_FILE_NOT_FOUND;
                    strcpy(resp.message, "File could not be written to");
                    send(client_socket, &resp, sizeof(Response), 0);
                    break;
                }
                // SERVER WILL SEND A RESPONSE AS TO FEASIBILITY OF WRITING.
                // this will be based on locking and whatnot,
                // whether there's already somebody writing to that file or things like that.
                // remember async write, so for a file, 
                //only one client can have access to that file at that point
                resp.status = ss_response.status;
                strcpy(resp.message, ss_response.message);
                resp.ss_port = ss->client_port;
                strcpy(resp.ss_ip,ss->ss_ip);
                send(client_socket, &resp, sizeof(Response), 0);
                // The parent folder exists
                break;
            }
        // Handle other commands
        case CMD_GET_INFO:
            filepath = msg.path;
            ss = find_file_location(filepath);
            if(ss == NULL)
            {
                printf("File not found\n");
                log_nm_operation("File not found");
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                if(send(client_socket, &resp, sizeof(Response), 0) < 0)
                {
                    log_nm_operation("Error sending response to client");
                    printf("Error sending response to client\n");
                }
                break;
            }
            else
            {
                // now we can send the request to the storage server.
                // we just forward the request to the storage server.
                if((send(ss->sock_fd,&msg,sizeof(msg),0))<0)
                {
                    printf("send failed\n");
                }
                // now we can receive the response from the storage server.
                if(recv(ss->sock_fd, &ss_response, sizeof(Response), 0)<0)
                {
                    //error handling
                    resp.status = STATUS_ERR_FILE_NOT_FOUND;
                    strcpy(resp.message, "File data could not be accessed");
                    send(client_socket, &resp, sizeof(Response), 0);
                    break;
                }
                // SERVER WILL SEND A RESPONSE AS TO FEASIBILITY OF WRITING.
                // this will be based on locking and whatnot,
                // whether there's already somebody writing to that file or things like that.
                // remember async write, so for a file, 
                //only one client can have access to that file at that point
                resp.status = ss_response.status;
                strcpy(resp.message, ss_response.message);
                resp.ss_port = ss->client_port;
                strcpy(resp.ss_ip,ss->ss_ip);
                send(client_socket, &resp, sizeof(Response), 0);
                // The parent folder exists
                break;
            }
            break;
        case CMD_CREATE:
            // this should only be when the user sends CREATE FILENAME, the 
            //actual file creation should be handled by the storage server and user.
            //this is just to add the file to the trie.

            // I dont think we should be establishing a client-server connection here.

            // the way the client will send information is as follows, the client will send the path of the file
            // the name of the file, and information about the filetype,ie as to whether it's a file or folder
            // The filepath will be in msg.path, the filename will be in msg.data.
            // THE ENTIRE FILEPATH WILL BE IN msg.path, 
            // its name will be in msg.data, and its type will be in msg.flags.
            // The client will send the message in the following format:
            int filetype = msg.flags;
            char* parentpath = msg.path;
            char* filename = msg.data;
            if(filetype!=0 && filetype!=1)
            {
                // THIS SHOULD NOT BE POSSIBLE. THIS IS AN ERROR.
                resp.status = STATUS_ERR_INVALID_COMMAND;
                strcpy(resp.message, "Invalid flag received, flag must be 0 or 1.");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }
            // Now, we have the path, filename and filetype. We can add the file to the trie.
            // We can add the file to the trie by calling the add_file function.
            // checking if the parent path exists.
            int len = strlen(msg.path);
            if(msg.path[len-1]=='/')
            {
                char *dir_path = calloc(len+2,sizeof(char));
                strcpy(dir_path,msg.path);
                dir_path[len-1] = '\0';
                destss = find_file_location(dir_path);
            }
            else
                destss = find_file_location(msg.path);
            if(destss==NULL)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message, "File could not be created");
                log_nm_operation("File could not be created");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }

            // // Send message to client that the creation is feasible.
            // resp.status = STATUS_SUCCESS;
            // strcpy(resp.message, "File creation is feasible");
            // send(client_socket, &resp, sizeof(Response), 0);

            // now we can send the request to create the file to the storage server.
            // Handle CREATE command
            int create_status = handle_create(destss, &msg);
            if(create_status==0)
            {
                resp.status = STATUS_SUCCESS;
                strcpy(resp.message, "File created successfully");
                send(client_socket, &resp, sizeof(Response), 0);
                // now we can add the file to the trie.
                char* absolutepath =(char*)calloc(strlen(parentpath)+strlen(filename)+2,sizeof(char));
                strcpy(absolutepath,parentpath);
                if (parentpath[strlen(parentpath) - 1] != '/')
                    strcat(absolutepath,"/");
                strcat(absolutepath,filename);
                log_nm_operation("File created successfully");

                // With destss, and myssl, get the index of the storage server.
                int idx = get_index(destss);
                if (idx == -1) {
                    perror("Error getting index of storage server");
                    break;
                }
                add_file(absolutepath, idx);
            }
            else
            {
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                printf("File could not be created\n");
                printf("%s\n", resp.message);
                strcpy(resp.message, "File could not be created");
                send(client_socket, &resp, sizeof(Response), 0);
            }
            break;
        
        case CMD_DELETE:
            // Same input format as CREATE
            // Delete Filepath Filename(Can also be a folder ??)

            char searchpath[MAX_PATH_LENGTH];
            strcpy(searchpath, msg.path);
            if (msg.path[strlen(msg.path) - 1] != '/')
                strcat(searchpath,"/");
            strcat(searchpath,msg.data);
            destss = find_file_location(searchpath);
            if(destss==NULL)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message, "File could not be deleted");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }
            printf("File/folder found\n");
            log_nm_operation("File/folder found");

            int delete_status = handle_delete(destss, &msg);
            
            printf("Delete status: %d\n", delete_status);
            if(delete_status==0)
            {
                resp.status = STATUS_SUCCESS;
                strcpy(resp.message, "File deleted successfully");
                send(client_socket, &resp, sizeof(Response), 0);
                // now we can remove the file from the trie.
                char* absolutepath =(char*)calloc(strlen(msg.path)+strlen(msg.data)+2,sizeof(char));
                strcpy(absolutepath,msg.path);
                if (msg.path[strlen(msg.path) - 1] != '/')
                    strcat(absolutepath,"/");
                strcat(absolutepath,msg.data);

                remove_file(absolutepath);
                log_nm_operation("File deleted successfully");
            }
            else if (delete_status==-2)
            {
                resp.status = STATUS_ERR_WRITE_CONFLICT;
                strcpy(resp.message, "File is currently in use");
                send(client_socket, &resp, sizeof(Response), 0);
                log_nm_operation("File is currently in use");
            }
            else
            {
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message, "File could not be deleted");
                send(client_socket, &resp, sizeof(Response), 0);
                log_nm_operation("File could not be deleted");
            }
            break;
        case CMD_COPY_SEND:
            //  so the received message should contain source in path, and destination in data.
            // NOTE THE DESTINATION PATH
            // SHOULD BE THE location to where the file is to be copied.
            // for source we consider the entire path, 
            // for destination, 
            // we consider everything uptil the last /, 
            // ie; checking if where the parent folder exists. 
            StorageServer* srcss = find_file_location(msg.path);
            char* destpath = (char*)calloc(MAX_PATH_LENGTH+3,sizeof(char));
            char* slashpoint = strrchr(msg.data,'/');
            strncpy(destpath,msg.data,slashpoint-msg.data);
            // now we search for the parent folder, in the copy destination.
            destss = find_file_location(msg.data);
            if(srcss==NULL || destss==NULL)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message, "File could not be copied");
                log_nm_operation("File could not be copied");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }
            // so now we send a message to both the source and recipient storage servers.
            Message* srcmsg = (Message*)calloc(1,sizeof(Message));
            srcmsg->command = CMD_COPY_SEND;
            // SOURCE PATH IN MSG->PATH
            strcpy(srcmsg->path,msg.path);
            // DESTINATION PATH IN MSG->DATA
            strcpy(srcmsg->data,msg.data);
            //now we're gonna put the destn server information in the source server message.
            strcpy(srcmsg->dest_ip,destss->ss_ip);
            // the port on the dest to which the message will be forwarded to is the client port.
            srcmsg->dest_port = destss->client_port;
            
            Message* destmsg = (Message*)calloc(1,sizeof(Message));
            destmsg->command = CMD_COPY_RECV;
            // SOURCE PATH IN MSG->PATH
            strcpy(destmsg->path,msg.path);
            // DESTINATION PATH IN MSG->DATA
            strcpy(destmsg->data,msg.data);
            //now we're gonna put the source server information in the destn server message.?
            strcpy(destmsg->dest_ip,srcss->ss_ip);
            // the port on the dest to which the message will be forwarded to is the client port.
            destmsg->dest_port = srcss->client_port;
            
            //send the message to the destn server,ask if its possible
            send(destss->sock_fd,destmsg,sizeof(Message),0);
            //now receive the response as to whether its possible or not.
            Response dest_response;
            if(recv(destss->sock_fd, &dest_response, sizeof(Response), 0)<0)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                log_nm_operation("File could not be copied.");
                strcpy(resp.message, "File could not be copied");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }
            // now if copying over said file is possible to the destination.
            // we can send the request to the source server.
            //ideally since we already checked the trie for the file, we should be able to send the message.
            if(send(srcss->sock_fd,srcmsg,sizeof(Message),0)<0)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                log_nm_operation("File could not be copied.");
                strcpy(resp.message, "File could not be copied");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }
            Response srcresp;
            if(recv(srcss->sock_fd, &srcresp, sizeof(Response), 0)<0)
            {
                //error handling
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                log_nm_operation("File could not be copied.");
                strcpy(resp.message, "File could not be copied");
                send(client_socket, &resp, sizeof(Response), 0);
                break;
            }

            // SERVER WILL SEND A RESPONSE AS TO FEASIBILITY OF COPYING.
            // this will be based on locking and whatnot,
            if(srcresp.status==STATUS_SUCCESS && dest_response.status==STATUS_SUCCESS)
            {
                resp.status = STATUS_SUCCESS;
                strcpy(resp.message, "File copied successfully");
                send(client_socket, &resp, sizeof(Response), 0);
                log_nm_operation("File copied successfully.");
                get_directory_structure(files, msg.path);
                int num_files = die->index;
                char **file_paths = die->file_names;
                
                char** file_paths_copy = (char**)calloc(num_files+1,sizeof(char*));
                
                // Check if the source and destination are the same
                if (strcmp(srcss->ss_ip, destss->ss_ip) == 0 && srcss->client_port == destss->client_port) {
                    // If the source and destination are the same, the new files will need to be added to 
                    // the trie as members of the source server.
                    
                    // Start with the msg.path; are there any files in the trie which start with this path?
                    // Keep all of these files in a list.
                    for(int i =0;i<num_files;i++)
                    {
                        file_paths_copy[i] = (char*)calloc(strlen(file_paths[i])+1,sizeof(char));
                        strcpy(file_paths_copy[i],msg.data);
                        strcat(file_paths_copy[i],file_paths[i]+strlen(msg.path));
                        add_file(file_paths_copy[i],get_index(srcss));
                    }

                    if (num_files == 0) {
                        // The msg.path contains a file. Get the filename, concatenate with msg.data and add to trie.
                        
                        // Check if msg.data ends with a /, if not add it,
                        if (msg.data[strlen(msg.data) - 1] != '/')
                            strcat(msg.data, "/");

                        char* filename = basename(msg.path);
                        char* file_path = (char*) calloc(strlen(msg.data) + strlen(filename) + 1, sizeof(char));

                        strcpy(file_path, msg.data);
                        strcat(file_path, filename);

                        add_file(file_path, get_index(srcss));
                    }

                } else {
                    // the source and destination are not the same, the new files will need to be added to 
                    // the trie as members of the destination server.
                    for(int i =0;i<num_files;i++)
                    {
                        file_paths_copy[i] = (char*)calloc(2*MAX_PATH_LENGTH,sizeof(char));
                        strcpy(file_paths_copy[i],msg.data);
                        strcat(file_paths_copy[i],"/");
                        strcat(file_paths_copy[i],basename(file_paths[i]));
                        add_file(file_paths_copy[i],get_index(destss));
                    }

                    if (num_files == 0) {
                        // The msg.path contains a file. Get the filename, concatenate with msg.data and add to trie.
                        
                        // Check if msg.data ends with a /, if not add it,
                        if (msg.data[strlen(msg.data) - 1] != '/')
                            strcat(msg.data, "/");

                        char* filename = basename(msg.path);
                        char* file_path = (char*) calloc(strlen(msg.data) + strlen(filename) + 1, sizeof(char));

                        strcpy(file_path, msg.data);
                        strcat(file_path, filename);

                        add_file(file_path, get_index(destss));
                    }
                }
                
            }
            else
            {
                resp.status = STATUS_ERR_FILE_NOT_FOUND;
                strcpy(resp.message, "File could not be copied");
                log_nm_operation("File could not be copied.");
                send(client_socket, &resp, sizeof(Response), 0);
            }

            break;
            

        case CMD_LIST:
            // Implement LIST handling
            // Add all the files in the trie to a linked list.
            // Send the linked list to the client.
            // The client will then print the linked list.

            // Iterate over the trie and add all the files to an array.
            // Send the array to the client.
            
            size_t num_files;
            char **file_paths = trie_keys(files, &num_files);

            resp.status = STATUS_SUCCESS;
            strcpy(resp.message, "List of files sent successfully");
            send(client_socket, &resp, sizeof(Response), 0);

            // Send the list of files to the client
            send(client_socket, &num_files, sizeof(int), 0);
            
            for (int i = 0; i < num_files; i++)
            {
                int len = strlen(file_paths[i]);
                if (send(client_socket, &len, sizeof(int), 0) < 0) {
                    perror("send - file path length");
                    break;
                }
                if (send(client_socket, file_paths[i], len, 0) < 0) {
                    perror("send - file path");
                    break;
                }
            }
            
            break;


        default:
            resp.status = STATUS_ERR_INVALID_COMMAND;
            strcpy(resp.message, "Invalid command received");
            log_nm_operation("Invalid command received.");
            send(client_socket, &resp, sizeof(Response), 0);
            break;
    }

    // Close the Client socket after handling
    // close(client_socket);
    pthread_exit(NULL);
}




