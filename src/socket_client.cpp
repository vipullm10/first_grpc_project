#include "socket_client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <vector>
#include "first_grpc_project.grpc.pb.h"

// These are defined here and declared extern in the header
grpc::ServerUnaryReactor *ptr = nullptr;
first_grpc_project::addResponse *ptr_res = nullptr;

int connect_to_server(const char *ip, int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket creation error");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
    {
        perror("invalid address / address not supported");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        close(sock);
        return -1;
    }
    std::cout << "Connected to server" << std::endl;
    return sock;
}

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
                if (ptr_res && ptr)
                {
                    ptr_res->set_result(response_vector[j].result);
                    ptr->Finish(grpc::Status::OK);
                }
            }
        }
    }

    return nullptr;
}