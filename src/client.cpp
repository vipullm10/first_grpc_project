#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using first_grpc_project::Adder;
using first_grpc_project::addRequest;
using first_grpc_project::addResponse;

class AdderClient {
public:
    AdderClient(std::shared_ptr<Channel> channel)
        : stub_(Adder::NewStub(channel)) {}

    [[nodiscard]] int Add(int num1, int num2) {
        // Prepare request
        addRequest request;
        request.set_num1(num1);
        request.set_num2(num2);

        // Response container
        addResponse response;

        // Client context for optional settings (e.g., timeouts)
        ClientContext context;

        // Make the RPC call
        Status status = stub_->add(&context, request, &response);

        if (status.ok()) {
            std::cout << "Server returned: " << response.result() << std::endl;
            return response.result();
        } else {
            std::cerr << "RPC failed: " << status.error_code()
                      << " - " << status.error_message() << std::endl;
            return -1;
        }
    }

    bool getMultiplicationTable(int num,int n){
        first_grpc_project::tableRequest request;
        request.set_num(num);
        request.set_n(n);

        //response container
        first_grpc_project::tableResponse res;

        //Client context for optional settings (e.g. timeouts)
        ClientContext context;

        //Make the RPC Call
        std::unique_ptr<grpc::ClientReader<first_grpc_project::tableResponse>> reader = stub_->getMultiplicationTable(&context,request);
        while(reader->Read(&res)){
            std::cout<<res.num()<<" * "<<res.n()<<" = "<<res.result()<<std::endl;
        }
        Status status = reader->Finish();
        if(status.ok()){
            std::cout<<"RPC Finished Successfully"<<std::endl;
            return true;
        }
        std::cout<<"RPC Failed"<<std::endl;
        return false;
    }

private:
    std::unique_ptr<Adder::Stub> stub_;
};

int main(int argc, char** argv) {
    // Connect to server
    AdderClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    int num1 = 7;
    int num2 = 5;
    if (argc == 3) {
        num1 = std::stoi(argv[1]);
        num2 = std::stoi(argv[2]);
    }

    std::cout << "Sending request: " << num1 << " + " << num2 << std::endl;
    int res = client.Add(num1, num2);

    std::cout<<"Sending getMultiplicationTable request : "<<res<<" "<<10<<std::endl;
    bool ret = client.getMultiplicationTable(res,10);
    

    return 0;
}
