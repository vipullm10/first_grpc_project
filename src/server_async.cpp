#include <iostream>
#include <grpcpp/grpcpp.h>
#include <vector>
#include <pthread.h>
#include "first_grpc_project.grpc.pb.h"
#include "socket_client.h"

using grpc::Server;
using grpc::ServerBuilder;

// Use externs from socket_client.cpp
extern grpc::ServerUnaryReactor *ptr;
extern first_grpc_project::addResponse *ptr_res;
int fd;

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
                ptr = this;
                ptr_res = res;
                request req_;
                req_.num1 = req->num1();
                req_.num2 = req->num2();
                int sent_bytes = send(fd, &req_, sizeof(request), 0);
                std::cout << "sent_bytes : " << sent_bytes << " error code : " << errno << std::endl;
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
