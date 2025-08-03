#ifndef __NAMING_SERVER_H
#define __NAMING_SERVER_H

#include<string.h>


//Initializes the naming server with the public ip and port.
void initialize_nm(const char* public_ip, int public_port);


// Initializes the storage server registration.
//This will initialize the data structure the nm has access to.
void intialize_ss_registration();

//true argument is the server socket.Establish the connection with the server.
// Following this receive the storage server as a struct sent by the server.
// If this mesasge is received, then the naming server will store the storage server in its list of storage servers.
// The naming server will then send a message to the server that the storage server has been registered.
// Based on recency or something, might cache the storage server in the naming server.
void* handle_ss_registration(void* arg);


//Actual arguemnt is the client socket. This function will handle the client request.
//Client request will be sent as a request struct.
// based on the request, the naming server will send a response to the client.
void* handle_client_request(void *arg);


#endif // __NAMING_SERVER_H