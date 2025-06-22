// jwt_utils.cpp
#include "jwt_utils.hpp"
#include "util_functions.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>

JwtUtils::JwtUtils(const std::string &private_key_path, const std::string &public_key_path)
    : private_key_(LoadFile(private_key_path)), public_key_(LoadFile(public_key_path)) {}

std::string JwtUtils::GenerateToken(const std::string &username)
{
    auto token = jwt::create()
                     .set_issuer("my-auth-server")
                     .set_type("JWS")
                     .set_subject(username)
                     .set_issued_at(std::chrono::system_clock::now())
                     .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(30))
                     .sign(jwt::algorithm::rs256(public_key_, private_key_, "", ""));
    return token;
}

bool JwtUtils::ValidateToken(const std::string &token, std::string &out_username, std::string &error)
{
    try
    {
        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::rs256(public_key_, "", "", ""))
                            .with_issuer("my-auth-server");

        verifier.verify(decoded);

        out_username = decoded.get_subject();
        if (out_username.empty())
        {
            error = "missing 'sub' claim";
            return false;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        error = e.what();
        return false;
    }
}
