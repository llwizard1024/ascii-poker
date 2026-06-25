#include "storage/user_repository.h"

#include "storage/password_hash.h"

#include <poker/game_constants.h>

#include <spdlog/spdlog.h>

namespace poker::server {

namespace {

void bind_text(sqlite3_stmt* stmt, int index, const std::string& value)
{
    sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

} // namespace

UserRepository::UserRepository(Database& database)
    : database_(database)
{
}

UserCreateResult UserRepository::create_user(
    const std::string& username,
    const std::string& password,
    uint32_t chips)
{
    if (username_exists(username)) {
        return UserCreateResult::UsernameTaken;
    }

    const std::string password_hash = PasswordHasher::hash(password);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO users (username, password_hash, chips) VALUES (?, ?, ?)";
    sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr);
    bind_text(stmt, 1, username);
    bind_text(stmt, 2, password_hash);
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(chips));
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_CONSTRAINT) {
        return UserCreateResult::UsernameTaken;
    }
    if (rc != SQLITE_DONE) {
        throw DatabaseError("create_user failed: " + std::string(sqlite3_errmsg(database_.handle())));
    }

    return UserCreateResult::Created;
}

std::optional<UserRecord> UserRepository::find_by_username(const std::string& username) const
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, username, chips FROM users WHERE username = ? COLLATE NOCASE LIMIT 1";
    sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr);
    bind_text(stmt, 1, username);

    std::optional<UserRecord> record;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRecord user;
        user.id = sqlite3_column_int64(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.chips = static_cast<uint32_t>(sqlite3_column_int64(stmt, 2));
        record = std::move(user);
    }

    sqlite3_finalize(stmt);
    return record;
}

bool UserRepository::verify_password(const std::string& username, const std::string& password) const
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT password_hash FROM users WHERE username = ? COLLATE NOCASE LIMIT 1";
    sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr);
    bind_text(stmt, 1, username);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        ok = hash != nullptr && PasswordHasher::verify(password, hash);
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool UserRepository::update_chips(const std::string& username, uint32_t chips)
{
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "UPDATE users SET chips = ?, updated_at = datetime('now') WHERE username = ? COLLATE NOCASE";
    const int prepare_rc = sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr);
    if (prepare_rc != SQLITE_OK) {
        spdlog::error(
            "update_chips prepare failed for '{}': {}",
            username,
            sqlite3_errmsg(database_.handle()));
        return false;
    }

    sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(chips));
    bind_text(stmt, 2, username);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        spdlog::error(
            "update_chips failed for '{}': {}",
            username,
            sqlite3_errmsg(database_.handle()));
        return false;
    }

    if (sqlite3_changes(database_.handle()) != 1) {
        spdlog::warn("update_chips matched no rows for '{}'", username);
        return false;
    }

    return true;
}

bool UserRepository::username_exists(const std::string& username) const
{
    return find_by_username(username).has_value();
}

} // namespace poker::server
