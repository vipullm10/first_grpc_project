#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"
#include "ServiceImpl.h"
#include "socket_client.h"

extern int fd;
// These are used by the callback to communicate with gRPC
extern grpc::ServerUnaryReactor *ptr;
extern first_grpc_project::addResponse *ptr_res;

grpc::ServerUnaryReactor *ServiceImpl::add(grpc::CallbackServerContext *context, const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res)
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

grpc::ServerWriteReactor<first_grpc_project::tableResponse> *ServiceImpl::getMultiplicationTable(grpc::CallbackServerContext *context, const first_grpc_project::tableRequest *req)
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
