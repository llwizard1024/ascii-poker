#pragma once

#include <string>
#include <string_view>

namespace poker::server {

class PasswordHasher {
public:
    static std::string hash(std::string_view password);
    static bool verify(std::string_view password, std::string_view stored_hash);
};

} // namespace poker::server
