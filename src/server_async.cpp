#include <iostream>
#include <grpcpp/grpcpp.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <vector>
#include "first_grpc_project.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;

#define PORT 8081
#define BUFFER_SIZE 1024

grpc::ServerUnaryReactor *ptr;
first_grpc_project::addResponse *ptr_res;
int fd;

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

// Thread that polls the client socket for server responses
void *client_read_callback(void *args)
{
    int server_fd = ((client_thread_args *)args)->server_fd;

    struct pollfd pfd;
    struct buffer_info bufinfo = {};
    size_t msg_size = sizeof(response);

    pfd.fd = server_fd;
    pfd.events = POLLIN;

    while (true)
    {
        std::cout << "Polling..." << std::endl;
        int poll_count = poll(&pfd, 1, -1);
        if (poll_count < 0)
        {
            perror("poll");
            break;
        }

        if (pfd.revents & POLLIN)
        {
            ssize_t count = read(server_fd, bufinfo.buffer + bufinfo.bytes_read,
                                 BUFFER_SIZE - bufinfo.bytes_read);
            if (count <= 0)
            {
                if (count == 0)
                {
                    std::cout << "Server disconnected" << std::endl;
                }
                else
                {
                    perror("read");
                }
                close(server_fd);
                break;
            }

            std::vector<response> response_vector;
            bufinfo.bytes_read += count;
            std::cout << "bytes_read: " << bufinfo.bytes_read << ", msg_size: " << msg_size << std::endl;

            int index = 0;
            while (bufinfo.bytes_read >= msg_size)
            {
                response *response_obj = (response *)(bufinfo.buffer + index);
                response_vector.push_back(*response_obj);
                bufinfo.bytes_read -= msg_size;
                index += msg_size;
            }

            memmove(bufinfo.buffer, bufinfo.buffer + index, bufinfo.bytes_read);

            for (size_t j = 0; j < response_vector.size(); ++j)
            {
                std::cout << "RESPONSE " << j + 1 << " : { result: " << response_vector[j].result << " }" << std::endl;
                ptr_res->set_result(response_vector[j].result);
                ptr->Finish(grpc::Status::OK);
            }
        }
    }

    return nullptr;
}

class ServiceImpl final : public first_grpc_project::Adder::CallbackService
{
public:
    grpc::ServerUnaryReactor *add(grpc::CallbackServerContext *context, const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res) override
    {
        class Reactor : public grpc::ServerUnaryReactor
        {
        public:
            Reactor(const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res)
            {
                // res->set_result(req->num1() + req->num2());
                ptr = this;
                ptr_res = res;
                request req_;
                req_.num1 = req->num1();
                req_.num2 = req->num2();
                int sent_bytes = send(fd, &req_, sizeof(request), 0);
                std::cout << "sent_bytes : " << sent_bytes << " error code : " << errno << std::endl;
                // Finish(grpc::Status::OK);
            }

        private:
            void OnDone() override
            {
                std::cout << "RPC Finished" << std::endl;
                delete this;
            }
        };
        return new Reactor(req, res);
    }

    grpc::ServerWriteReactor<first_grpc_project::tableResponse> *getMultiplicationTable(grpc::CallbackServerContext *context, const first_grpc_project::tableRequest *req) override
    {
        class Multiplier : public grpc::ServerWriteReactor<first_grpc_project::tableResponse>
        {
        public:
            Multiplier(const first_grpc_project::tableRequest *req) : req_(req), i(1), res(new first_grpc_project::tableResponse)
            {
                NextWrite();
            }
            ~Multiplier()
            {
                delete res;
            }

        private:
            void NextWrite()
            {
                if (i > req_->n())
                {
                    Finish(grpc::Status::OK);
                    return;
                }
                res->set_n(i);
                res->set_num(req_->num());
                res->set_result(req_->num() * i);
                StartWrite(res);
                i++;
            }
            void OnWriteDone(bool ok) override
            {
                if (!ok)
                {
                    Finish(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
                    return;
                }
                NextWrite();
            }
            void OnDone() override
            {
                std::cout << "RPC Completed";
                delete this;
            }
            void OnCancel() override
            {
                std::cout << "RPC Cancelled" << std::endl;
            }
            int i;
            const first_grpc_project::tableRequest *req_;
            first_grpc_project::tableResponse *res;
        };
        return new Multiplier(req);
    }
};

void RunServer()
{
    int sock = 0;
    struct sockaddr_in serv_addr;

    std::string server_address("0.0.0.0:50051");
    ServiceImpl service;

    // 1. Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket creation error");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        perror("invalid address / address not supported");
        return;
    }

    // 2. Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        return;
    }
    std::cout << "Connected to server" << std::endl;
    fd = sock;

    pthread_t client_reader_thread_id;
    client_thread_args args;
    args.server_fd = sock;

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

    // 5. Close socket
    close(sock);
    return;
}

int main(int, char **argc)
{
    RunServer();
    return 0;
}
