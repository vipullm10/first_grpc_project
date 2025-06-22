// jwt_utils.hpp
#pragma once

#include <string>
#include <jwt-cpp/jwt.h>

class JwtUtils
{
public:
    JwtUtils(const std::string &private_key_path, const std::string &public_key_path);

    std::string GenerateToken(const std::string &username);
    bool ValidateToken(const std::string &token, std::string &out_username, std::string &error);

private:
    std::string private_key_;
    std::string public_key_;
};
