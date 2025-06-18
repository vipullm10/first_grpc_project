#include <iostream>
#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
 

class ServiceImpl final : public first_grpc_project::Adder::Service {
public:
    ::grpc::Status add(::grpc::ServerContext* context,
                       const ::first_grpc_project::addRequest* request,
                       ::first_grpc_project::addResponse* response) override { 
        
        int num1 = request->num1();  
        int num2 = request->num2();
        int result = num1 + num2;

        std::cout << "Received request to add two numbers: "
                  << num1 << " + " << num2 << " = " << result << std::endl;

        response->set_result(result);  // Set the response field

        return ::grpc::Status::OK;
    }

    ::grpc::Status getMultiplicationTable(::grpc::ServerContext* context, const ::first_grpc_project::tableRequest* request, ::grpc::ServerWriter< ::first_grpc_project::tableResponse>* writer) override{
        for(int i=1;i<=request->n();i++){
            first_grpc_project::tableResponse res;
            res.set_num(request->num());
            res.set_n(i);
            res.set_result(request->num()*i);
            writer->Write(res);
        }
        return ::grpc::Status::OK;
    }

};


void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}


int main(int, char** argc){
    RunServer();
    return 0;
}
