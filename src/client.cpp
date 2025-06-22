#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"
#include "util_functions.h"

using first_grpc_project::Adder;
using first_grpc_project::addRequest;
using first_grpc_project::addResponse;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class AdderClient
{
public:
    AdderClient(std::shared_ptr<Channel> channel)
        : stub_(Adder::NewStub(channel)) {}

    [[nodiscard]] int Add(int num1, int num2, std::string jwt_token)
    {
        // Prepare request
        addRequest request;
        request.set_num1(num1);
        request.set_num2(num2);

        // Response container
        addResponse response;

        // Client context for optional settings (e.g., timeouts)
        ClientContext context;
        context.AddMetadata("authorization", "Bearer " + jwt_token); // Add JWT token to metadata

        // Make the RPC call
        Status status = stub_->add(&context, request, &response);

        if (status.ok())
        {
            std::cout << "Server returned: " << response.result() << std::endl;
            return response.result();
        }
        else
        {
            std::cerr << "RPC failed: " << status.error_code()
                      << " - " << status.error_message() << std::endl;
            return -1;
        }
    }

    bool getMultiplicationTable(int num, int n, std::string jwt_token)
    {
        first_grpc_project::tableRequest request;
        request.set_num(num);
        request.set_n(n);

        // response container
        first_grpc_project::tableResponse res;

        // Client context for optional settings (e.g. timeouts)
        ClientContext context;
        context.AddMetadata("authorization", "Bearer " + jwt_token); // Add JWT token to metadata

        // Make the RPC Call
        std::unique_ptr<grpc::ClientReader<first_grpc_project::tableResponse>> reader = stub_->getMultiplicationTable(&context, request);
        while (reader->Read(&res))
        {
            std::cout << res.num() << " * " << res.n() << " = " << res.result() << std::endl;
        }
        Status status = reader->Finish();
        if (status.ok())
        {
            std::cout << "RPC Finished Successfully" << std::endl;
            return true;
        }
        std::cout << "RPC Failed: " << status.error_code() << " - " << status.error_message() << std::endl;
        return false;
    }

    std::string login(const std::string &username, const std::string &password)
    {
        first_grpc_project::loginRequest request;
        request.set_username(username);
        request.set_password(password);

        first_grpc_project::loginResponse response;

        ClientContext context;

        std::cout << "Sending login request for user: " << username << std::endl;
        Status status = stub_->login(&context, request, &response);

        if (status.ok())
        {
            return response.jwt_token();
        }
        else
        {
            std::cerr << "Login failed: " << status.error_code()
                      << " - " << status.error_message() << std::endl;
            return "";
        }
    }

private:
    std::unique_ptr<Adder::Stub> stub_;
};

int main(int argc, char **argv)
{
    // Connect to server
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = LoadFile("/Users/vipuldevnani/grpc_certs/ca.crt");      // To verify server cert
    ssl_opts.pem_private_key = LoadFile("/Users/vipuldevnani/grpc_certs/client.key"); // Client's private key
    ssl_opts.pem_cert_chain = LoadFile("/Users/vipuldevnani/grpc_certs/client.crt");  // Client's cert

    std::shared_ptr<grpc::ChannelCredentials> creds = grpc::SslCredentials(ssl_opts);

    AdderClient client(grpc::CreateChannel("localhost:50051", creds));

    std::cout << "Logging in with username: admin and password: password" << std::endl;
    std::string jwt_token = client.login("admin", "password");

    int num1 = 7;
    int num2 = 5;
    if (argc == 3)
    {
        num1 = std::stoi(argv[1]);
        num2 = std::stoi(argv[2]);
    }

    std::cout << "Sending request: " << num1 << " + " << num2 << std::endl;
    int res = client.Add(num1, num2, jwt_token);

    std::cout << "Sending getMultiplicationTable request : " << res << " " << 10 << std::endl;
    bool ret = client.getMultiplicationTable(res, 10, jwt_token);
    std::cout << "BYE" << std::endl;
    return 0;
}
