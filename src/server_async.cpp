#include <iostream>
#include <grpcpp/grpcpp.h>
#include <vector>
#include <pthread.h>
#include "first_grpc_project.grpc.pb.h"
#include "socket_client.h"
#include "ServiceImpl.h"

using grpc::Server;
using grpc::ServerBuilder;

// Use externs from ServiceImpl.cpp
extern grpc::ServerUnaryReactor *ptr;
extern first_grpc_project::addResponse *ptr_res;
int fd;

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    ServiceImpl service;

    fd = connect_to_server("127.0.0.1", PORT);
    if (fd < 0)
        return;

    pthread_t client_reader_thread_id;
    client_thread_args args;
    args.server_fd = fd;

    if (pthread_create(&client_reader_thread_id, NULL, client_read_callback, (void *)&args) != 0)
    {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    server->Wait();
    void *ret_val;
    pthread_join(client_reader_thread_id, &ret_val);

    std::cout << "Thread finished execution" << std::endl;
    close(fd);
    return;
}

int main(int, char **)
{
    RunServer();
    return 0;
}
