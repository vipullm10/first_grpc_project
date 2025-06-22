#include <iostream>
#include <grpcpp/grpcpp.h>
#include <vector>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include "first_grpc_project.grpc.pb.h"
#include "socket_client.h"
#include "ServiceImpl.h"
#include "util_functions.h"

using grpc::Server;
using grpc::ServerBuilder;

// Use externs from ServiceImpl.cpp
extern grpc::ServerUnaryReactor *ptr;
extern first_grpc_project::addResponse *ptr_res;
int fd;

void RunServer()
{
    std::string server_address("localhost:50051");
    const std::string private_key_path = "/Users/vipuldevnani/keys/private.pem";
    const std::string public_key_path = "/Users/vipuldevnani/keys/public.pem";
    ServiceImpl service(private_key_path, public_key_path);

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

    grpc::SslServerCredentialsOptions::PemKeyCertPair server_pair = {
        LoadFile("/Users/vipuldevnani/grpc_certs/server.key"), LoadFile("/Users/vipuldevnani/grpc_certs/server.crt")};

    grpc::SslServerCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = LoadFile("/Users/vipuldevnani/grpc_certs/ca.crt"); // To verify client certs
    ssl_opts.pem_key_cert_pairs.push_back(server_pair);

    // Require client auth
    ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;

    std::shared_ptr<grpc::ServerCredentials> creds = grpc::SslServerCredentials(ssl_opts);

    ServerBuilder builder;
    builder.AddListeningPort(server_address, creds);
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
