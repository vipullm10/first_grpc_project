#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"

class ServiceImpl final : public first_grpc_project::Adder::CallbackService
{
public:
    grpc::ServerUnaryReactor *add(grpc::CallbackServerContext *context, const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res) override;

    grpc::ServerWriteReactor<first_grpc_project::tableResponse> *getMultiplicationTable(grpc::CallbackServerContext *context, const first_grpc_project::tableRequest *req) override;
};
