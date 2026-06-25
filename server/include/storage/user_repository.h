#pragma once

#include "storage/database.h"

#include <cstdint>
#include <optional>
#include <string>

namespace poker::server {

struct UserRecord {
    int64_t id = 0;
    std::string username;
    uint32_t chips = 0;
};

enum class UserCreateResult {
    Created,
    UsernameTaken,
};

class UserRepository {
public:
    explicit UserRepository(Database& database);

    UserCreateResult create_user(const std::string& username, const std::string& password, uint32_t chips);
    std::optional<UserRecord> find_by_username(const std::string& username) const;
    bool verify_password(const std::string& username, const std::string& password) const;
    bool update_chips(const std::string& username, uint32_t chips);
    bool username_exists(const std::string& username) const;

private:
    Database& database_;
};

} // namespace poker::server
