#include <grpcpp/grpcpp.h>
#include "first_grpc_project.grpc.pb.h"
#include "ServiceImpl.h"
#include "socket_client.h"

extern int fd;
// These are used by the callback to communicate with gRPC
extern grpc::ServerUnaryReactor *ptr;
extern first_grpc_project::addResponse *ptr_res;

grpc::ServerUnaryReactor *ServiceImpl::login(grpc::CallbackServerContext *context, const first_grpc_project::loginRequest *request, first_grpc_project::loginResponse *res)
{
    class Reactor : public grpc::ServerUnaryReactor
    {
    public:
        // Constructor just initializes members
        Reactor(const first_grpc_project::loginRequest *request_in, first_grpc_project::loginResponse *res_in, JwtUtils jwt_utils_in)
            : request_(request_in), res_(res_in), jwt_utils_(jwt_utils_in) {}

        // This method is called by gRPC after the reactor is instantiated.
        // It's the ideal place to perform the actual RPC logic.
        void Start()
        {
            std::cout << "Received login request for user: " << request_->username() << " password : " << request_->password() << std::endl;

            if (request_->username() == "admin" && request_->password() == "password")
            {
                std::cout << "Valid credentials provided." << std::endl;
                std::string token = jwt_utils_.GenerateToken(request_->username());
                res_->set_jwt_token(token);
                Finish(grpc::Status::OK); // Call Finish and let gRPC handle OnDone
            }
            else
            {
                std::cout << "Invalid credentials provided." << std::endl;
                Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid username or password")); // Call Finish and let gRPC handle OnDone
            }
        }

    private:
        void OnDone() override
        {
            std::cout << "Login RPC Finished" << std::endl;
            delete this; // Correct place to deallocate the reactor
        }

        const first_grpc_project::loginRequest *request_;
        first_grpc_project::loginResponse *res_;
        JwtUtils jwt_utils_;
    };

    // Instantiate the reactor and then call its Start method
    auto *reactor = new Reactor(request, res, this->getJwtUtils());
    reactor->Start(); // Initiate the login logic
    return reactor;   // Return the reactor to gRPC for lifecycle management
}

grpc::ServerUnaryReactor *ServiceImpl::add(grpc::CallbackServerContext *context, const first_grpc_project::addRequest *req, first_grpc_project::addResponse *res)
{
    class Reactor : public grpc::ServerUnaryReactor
    {
    public:
        Reactor(grpc::CallbackServerContext *context_, const first_grpc_project::addRequest *req_in, first_grpc_project::addResponse *res_in, JwtUtils jwt_utils_in) : req_(req_in), res_(res_in), jwt_utils_(jwt_utils_in)
        {
            // Constructor initializes members
        }

        void Start(grpc::CallbackServerContext *context_)
        {
            const auto &metadata = context_->client_metadata();
            auto auth_md = metadata.find("authorization");
            if (auth_md == metadata.end())
            {
                Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing authorization header"));
                return;
            }

            std::string auth_header(auth_md->second.begin(), auth_md->second.end());
            if (auth_header.rfind("Bearer ", 0) != 0)
            {
                Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid authorization format"));
                return;
            }

            std::string token = auth_header.substr(7); // Remove "Bearer "
            std::string username, error;
            if (!jwt_utils_.ValidateToken(token, username, error))
            {
                Finish(grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid token: " + error));
                return;
            }
            std::cout << "Valid token provided for user: " << username << std::endl;
            std::cout << "Received add request with num1: " << req_->num1() << " and num2: " << req_->num2() << std::endl;
            ptr = this;
            ptr_res = res_;
            request req_tcp;
            req_tcp.num1 = req_->num1();
            req_tcp.num2 = req_->num2();
            int sent_bytes = send(fd, &req_tcp, sizeof(request), 0);
            std::cout << "sent_bytes : " << sent_bytes << " error code : " << errno << std::endl;
        }

    private:
        void OnDone() override
        {
            std::cout << "RPC Finished" << std::endl;
            delete this;
        }
        const first_grpc_project::addRequest *req_;
        first_grpc_project::addResponse *res_;
        JwtUtils jwt_utils_;
    };
    auto *reactor = new Reactor(context, req, res, this->getJwtUtils());
    reactor->Start(context); // Start the reactor to handle the request
    return reactor;          // Return the reactor to gRPC for lifecycle management
}

grpc::ServerWriteReactor<first_grpc_project::tableResponse> *ServiceImpl::getMultiplicationTable(grpc::CallbackServerContext *context, const first_grpc_project::tableRequest *req)
{
    class Multiplier : public grpc::ServerWriteReactor<first_grpc_project::tableResponse>
    {
    public:
        Multiplier(const first_grpc_project::tableRequest *req_in, JwtUtils jwt_utils_in)
            : req_(req_in), i(1), res(new first_grpc_project::tableResponse), jwt_utils_(jwt_utils_in), initial_finish_status_(grpc::Status::OK) {}

        // This method is called by gRPC after the reactor is instantiated.
        // It's a better place to perform initial checks that might terminate the RPC.
        void Start(grpc::CallbackServerContext *context_)
        {
            const auto &metadata = context_->client_metadata();
            auto auth_md = metadata.find("authorization");
            if (auth_md == metadata.end())
            {
                initial_finish_status_ = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing authorization header");
                Finish(initial_finish_status_); // Finish now
                return;
            }

            std::string auth_header(auth_md->second.begin(), auth_md->second.end());
            if (auth_header.rfind("Bearer ", 0) != 0)
            {
                initial_finish_status_ = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid authorization format");
                Finish(initial_finish_status_); // Finish now
                return;
            }

            std::string token = auth_header.substr(7); // Remove "Bearer "
            std::string username, error;
            if (!jwt_utils_.ValidateToken(token, username, error))
            {
                initial_finish_status_ = grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid token: " + error);
                Finish(initial_finish_status_); // Finish now
                return;
            }

            std::cout << "Valid token provided for user: " << username << std::endl;
            std::cout << "Received multiplication table request for number: " << req_->num() << "*" << req_->n() << std::endl;
            NextWrite(); // Proceed with normal operation
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
            std::cout << "RPC Completed" << std::endl;
            delete this;
        }

        void OnCancel() override
        {
            std::cout << "RPC Cancelled" << std::endl;
            // OnDone will still be called after OnCancel, so no delete this here.
        }

        int i;
        const first_grpc_project::tableRequest *req_;
        first_grpc_project::tableResponse *res;
        JwtUtils jwt_utils_;
        grpc::Status initial_finish_status_; // To store status if finished early
    };

    // Instantiate and then call Start
    auto *multiplier = new Multiplier(req, this->getJwtUtils());
    multiplier->Start(context); // This initiates the RPC lifecycle checks
    return multiplier;
}