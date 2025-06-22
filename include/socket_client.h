#pragma once
#include <pthread.h>
#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"

#define PORT 8081
#define BUFFER_SIZE 1024

typedef struct request
{
    int num1;
    int num2;
} request;

typedef struct response
{
    int result;
} response;

typedef struct client_thread_args
{
    int server_fd;
} client_thread_args;

typedef struct buffer_info
{
    int fd;
    int bytes_read;
    char buffer[BUFFER_SIZE];
} buffer_info;

// Connects to the server and returns the socket fd, or -1 on error
int connect_to_server(const char *ip, int port);

// Thread function to read from the socket
void *client_read_callback(void *args);
