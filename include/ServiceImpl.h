#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"
#include "jwt_utils.hpp"

class ServiceImpl final : public first_grpc_project::Adder::CallbackService
{
public:
    ServiceImpl(const std::string &private_key_path, const std::string &public_key_path)
        : jwt_utils_(private_key_path, public_key_path) {}

    grpc::ServerUnaryReactor *login(grpc::CallbackServerContext *context, const first_grpc_project::loginRequest *request, first_grpc_project::loginResponse *res) override;

    grpc::ServerUnaryReactor *add(grpc::CallbackServerContext *context, const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res) override;

    grpc::ServerWriteReactor<first_grpc_project::tableResponse> *getMultiplicationTable(grpc::CallbackServerContext *context, const first_grpc_project::tableRequest *req) override;

    JwtUtils getJwtUtils() const
    {
        return jwt_utils_;
    }

private:
    JwtUtils jwt_utils_; // Adjust paths as necessary
};
