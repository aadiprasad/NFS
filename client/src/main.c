/*

1. Connect to the naming server - IP and port provided in args.
2. Be able to send a command to the naming server, client parses the command.
3. Early exit if the command is invalid.
4. Send the command to the naming server. (stub)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "linked-list.h"
#include "communication_protocols.h"

int connect_to_nm(char *nm_ip, int nm_port)
{
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return -1;
    }

    // Connect to the naming server
    struct sockaddr_in nm_addr;
    memset(&nm_addr, 0, sizeof(struct sockaddr_in));
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

    return sock;
}

Message *send_message(int sock_fd, char *cmd)
{
    if (send(sock_fd, "C", 1, 0) < 0)
    {
        perror("send");
        return NULL;
    }
    printf("C send after\n");

    // Parse the message, check if it is valid
    char *args[50];
    int n = 0;
    char *token = strtok(cmd, " ");
    while (token != NULL)
    {
        args[n++] = token;
        token = strtok(NULL, " ");
    }

    args[n] = NULL;

    if (args[0] == NULL)
    {
        printf("Invalid command\n");
        return NULL;
    }

    if (strcmp(args[0], "COPY") == 0)
    {
        if (n != 3)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else if (strcmp(args[0], "READ") == 0 || strcmp(args[0], "STREAM") == 0)
    {
        if (n != 2)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else if(strcmp(args[0], "WRITE") == 0 )
    {

    }
    else if (strcmp(args[0], "SHOW") == 0)
    {
        if (n != 1)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else if (strcmp(args[0], "CREATE") == 0)
    {
        if (n != 4 && n !=3)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else if (strcmp(args[0], "DELETE") == 0)
    {
        if (n != 3)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else if (strcmp(args[0], "GETINFO") == 0)
    {
        if (n != 2)
        {
            printf("Invalid command\n");
            return NULL;
        }
    }
    else
    {
        printf("Invalid command\n");
        return NULL;
    }
    // Create a Message struct using
    Message *message = (Message *)calloc(1, sizeof(Message));
    if (strcmp(args[0], "CREATE") == 0)
    {
        message->command = CMD_CREATE;
        strcpy(message->path, args[1]);
        strcpy(message->data, args[2]);
        if (args[3] != NULL)
        {
            message->flags = atoi(args[3]);
        }
        else
        {
            message->flags = 0;
        }
    }
    else if (strcmp(args[0], "DELETE") == 0)
    {
        message->command = CMD_DELETE;
        strcpy(message->path, args[1]);
        strcpy(message->data, args[2]);
    }

    else if (strcmp(args[0], "COPY") == 0)
    {
        message->command = CMD_COPY_SEND;
        strcpy(message->path, args[1]);
        strcpy(message->data, args[2]);
    }
    else if (strcmp(args[0], "READ") == 0)
    {
        message->command = CMD_READ;
        strcpy(message->path, args[1]);
        // No data for read
        message->data[0] = 0;
    }
    else if (strcmp(args[0], "STREAM") == 0)
    {
        message->command = CMD_STREAM;
        strcpy(message->path, args[1]);
        // No data for stream
        message->data[0] = 0;
    }
    else if(strcmp(args[0], "WRITE") == 0)
    {
        message->command = CMD_WRITE;
        strcpy(message->path, args[1]);
        // No data for WRITE
        message->data[0] = 0;
    }
    else if (strcmp(args[0], "SHOW") == 0)
    {
        message->command = CMD_LIST;
        // No path for show
        message->path[0] = 0;
        // No data for show
        message->data[0] = 0;
    }
    else if (strcmp(args[0], "GETINFO") == 0)
    {
        message->command = CMD_GET_INFO;
        strcpy(message->path, args[1]);
        message->data[0] = 0;
    }

    if (send(sock_fd, message, sizeof(Message), 0) < 0)
    {
        perror("send");
        return message;
    }
    return message;
}

#define CHUNK 4096

char dir[256];

char *rename_path2(char *curr_dir)
{
    int len_h = (int)strlen(dir);
    int len_c = (int)strlen(curr_dir);
    int len_a = len_h + len_c;

    char *abs_path = malloc(len_a + 1);
    abs_path[len_a] = 0;

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

char* strippath(char* filepath)
{
    int lastslash=0;
    int pathlen = strlen(filepath);
    for(int i=0;i<pathlen;i++)
        if(filepath[i]=='/')
            lastslash = i;
    
    char* newpath = malloc(sizeof(char)*(pathlen-lastslash+2));
    if(lastslash==pathlen)
        return "";

    for(int i=lastslash;i<pathlen;i++)
        newpath[i-lastslash]=filepath[i];
    newpath[pathlen-lastslash]='\0';
    return newpath;
}

char *rename_path3(char *curr_dir)
{
    // printf("%s is the path before any changes\n",curr_dir);
    // printf("%s is the stripped path\n",strippath(curr_dir));
    char *new_path = strippath(curr_dir);
    int len_h = (int)strlen(dir);
    int len_c = (int)strlen(new_path);
    int len_a = len_h + len_c;

    char *abs_path = malloc(len_a + 1);
    abs_path[len_a] = 0;

    for (int i = 0; i < len_h; i++)
    {
        abs_path[i] = dir[i];
    }
    for (int i = 0; i < len_c; i++)
    {
        abs_path[i + len_h] = new_path[i];
    }
    // char *temp = new_path;
    free(new_path);

    return abs_path;
}

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
            free(buffer);
            fclose(file);
            return -1;
        }
        total_sent += bytes_sent;
    }

    free(buffer);
    fclose(file);
    return total_sent;
}

int main(int argc, char const *argv[])
{
    getcwd(dir, 256);
    // args: nm_ip, nm_port
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <nm_ip> <nm_port>\n", argv[0]);
        return 1;
    }

    char *nm_ip = argv[1];
    int nm_port = atoi(argv[2]);

    // Send a command to the naming server
    while (1)
    {
        char cmd[256];
        printf("Enter a command: ");
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strcspn(cmd, "\n")] = 0;

        if (strcmp(cmd, "exit") == 0)
        {
            break;
        }
        printf("Command: %s\n", cmd);
        printf("before sending message\n");

        int sock_fd = connect_to_nm(nm_ip, nm_port);
        if (sock_fd < 0)
        {
            perror("couldn't connect to naming server\n");
            return 1;
        }

        // Send the command to the naming server
        Message *msg = send_message(sock_fd, cmd);
        if (msg == NULL)
        {
            printf("Message failed to send. Probably invalid command.\n");
            continue;
        }
        printf("after sending message\n");

        // this is response from the naming server as to whether the operation is feasible or not.
        Response rsp;
        ssize_t bytes_received = recv(sock_fd, &rsp, sizeof(Response), 0);
        if (bytes_received < 0)
        {
            printf("recv from naming server failed\n");
            perror("recv from naming server failed");
            close(sock_fd);
            free(msg);
            return -1;
        }
        printf("here\n");
        printf("feasibility: %d\n", rsp.status == STATUS_SUCCESS);
        printf("message from naming server: %s\n", rsp.message);
        if(rsp.status!=STATUS_SUCCESS)
        {
            printf("Operation not feasible\n");
            close(sock_fd);
            free(msg);
            continue;
        }
        // for copy,create and delete, we dont need to connnect to the storage server,
        // so we just  continue, ideally our output should simply be from the resp struct we receive.


        if (msg->command == CMD_LIST) {
            int num_files;
            if(recv(sock_fd, &num_files, sizeof(int), 0)<=0)
            {
                perror("Error receiving Storage Server details");
                close(sock_fd);
                return -1;
            }
            printf("SHOW HAS %d FILES\n", num_files);
            
            for (int i=0; i<num_files; i++) {
                int len;
                if(recv(sock_fd, &len, sizeof(int), 0)<=0)
                {
                    perror("Error receiving Storage Server details");
                    close(sock_fd);
                    return -1;
                }
                char *file_path = (char *)calloc(len+1, sizeof(char));
                if(recv(sock_fd, file_path, len, 0)<=0)
                {
                    perror("Error receiving Storage Server details");
                    close(sock_fd);
                    return -1;
                }
                file_path[len] = 0;
                printf("%s\n", file_path);
            } 
        
            close(sock_fd);
        }

        if (msg->command == CMD_CREATE || msg->command == CMD_COPY_SEND || msg->command == CMD_DELETE)
        {
            close(sock_fd);
            free(msg);
            continue;
        }

        else if(msg->command==CMD_GET_INFO)
        {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("socket");
                free(msg);
                return -1;
            }

            printf("%s\n%d\n", rsp.ss_ip, rsp.ss_port);
            // Connect to the storage server
            struct sockaddr_in ss_addr;
            memset(&ss_addr, 0, sizeof(ss_addr));
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(rsp.ss_port);
            if (inet_pton(AF_INET, rsp.ss_ip, &ss_addr.sin_addr) <= 0)
            {
                perror("inet_pton");
                free(msg);  
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) < 0)
            {
                perror("connect");
                free(msg);
                return -1;
            }
            printf("Connected to storage server at %s:%d\n", rsp.ss_ip, rsp.ss_port);


            if (send(sock, msg, sizeof(Message), 0) < 0)
            {
                perror("send");
                free(msg);
                return -1;
            }

            FileData2 data;
            recv(sock, &data, sizeof(FileData2), 0);
            printf("File size: %ld\nFile permissions: %s\n",data.file_size,data.permission);
            close(sock_fd);
            free(msg);
            continue;
        }
        // now for read and stream, we need to connect to the storage server.
        if (msg->command == CMD_READ || msg->command == CMD_STREAM|| msg->command == CMD_WRITE)
        {
            // transmit file in binary adn store it in the client server temporarily. elete at the end of the process based on client input ("press x to contue")
            // Create a socket, to connect to storage server.
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("socket");
                free(msg);
                return -1;
            }

            printf("%s\n%d\n", rsp.ss_ip, rsp.ss_port);
            // Connect to the storage server
            struct sockaddr_in ss_addr;
            memset(&ss_addr, 0, sizeof(ss_addr));
            ss_addr.sin_family = AF_INET;
            ss_addr.sin_port = htons(rsp.ss_port);
            if (inet_pton(AF_INET, rsp.ss_ip, &ss_addr.sin_addr) <= 0)
            {
                perror("inet_pton");
                free(msg);  
                return -1;
            }

            if (connect(sock, (struct sockaddr *)&ss_addr, sizeof(ss_addr)) < 0)
            {
                perror("connect");
                free(msg);
                return -1;
            }
            printf("Connected to storage server at %s:%d\n", rsp.ss_ip, rsp.ss_port);

            if (sock < 0)
            {
                perror("couldn't connect to storage server\n");
                free(msg);
                return 1;
            }

            // Send message to storage server, to actually request for the thing rn
            if (send(sock, msg, sizeof(Message), 0) < 0)
            {
                perror("send");
                free(msg);
                return -1;
            }
            receive_file(sock, rename_path3(msg->path));
            if (msg->command == CMD_READ)
            {
                printf("received file\n");
                char x;
                printf("press any key to continue: ");
                scanf("%c", &x);
                if(x!='\n')
                    getc(stdin);
                remove(rename_path3(msg->path));
            }
            else if(msg->command==CMD_STREAM)
            {
                // stream case
                // fork and exec ?
                char *command = (char *)calloc(1, 256);
                printf("path: %s\n", msg->path);
                snprintf(command, 256, "mpv %s", rename_path3(msg->path));
                printf("command: %s\n", command);
                int ret = system(command);
                if (ret == -1)
                {
                    perror("system");
                    free(msg);
                    return EXIT_FAILURE;
                }
                else
                {
                    printf("mpv exited with status %d\n", ret);
                }
                // deletes a file
                remove(rename_path3(msg->path));
            }
            else if(msg->command==CMD_WRITE)
            {   
                printf("received file\n");
                char x;
                printf("press any key to continue: ");
                scanf("%c",&x);
                if(x!='\n')
                    getc(stdin);
                send_file(sock, rename_path3(msg->path));
                remove(rename_path3(msg->path));
            }
            Response completion_ack; 
            completion_ack.status = STATUS_DONE; // to indicate complation of reading
            if (send(sock, &completion_ack, sizeof(Response), 0) < 0) {
                perror ("read incomplete"); 
            }
            close(sock_fd);
            free(msg);
        }
    }

    // Close the socket

    return 0;
}
