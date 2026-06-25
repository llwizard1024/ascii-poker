#include "storage/database.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdlib>
#include <filesystem>

#if defined(__linux__)
#include <climits>
#include <unistd.h>
#elif defined(__APPLE__)
#include <climits>
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace poker::server {

namespace {

void check_sqlite(int rc, sqlite3* db, const char* context)
{
    if (rc == SQLITE_OK || rc == SQLITE_DONE || rc == SQLITE_ROW) {
        return;
    }
    const char* message = db != nullptr ? sqlite3_errmsg(db) : sqlite3_errstr(rc);
    throw DatabaseError(std::string(context) + ": " + (message != nullptr ? message : "unknown error"));
}

std::filesystem::path executable_directory()
{
#if defined(__linux__)
    std::array<char, PATH_MAX> buffer {};
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length > 0) {
        buffer[static_cast<size_t>(length)] = '\0';
        return std::filesystem::path(buffer.data()).parent_path();
    }
#elif defined(__APPLE__)
    std::array<char, PATH_MAX> buffer {};
    uint32_t size = static_cast<uint32_t>(buffer.size());
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return std::filesystem::weakly_canonical(std::filesystem::path(buffer.data())).parent_path();
    }
#elif defined(_WIN32)
    std::array<wchar_t, MAX_PATH> buffer {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length > 0 && length < buffer.size()) {
        return std::filesystem::path(buffer.data()).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

} // namespace

std::string default_database_path()
{
    if (const char* env = std::getenv("ASCII_POKER_DB")) {
        if (env[0] != '\0') {
            return env;
        }
    }
    return (executable_directory() / "ascii-poker.db").lexically_normal().string();
}

Database::Database(std::string path)
    : path_(std::move(path))
{
    const int rc = sqlite3_open(path_.c_str(), &db_);
    check_sqlite(rc, db_, "sqlite3_open");
    sqlite3_busy_timeout(db_, 5000);
    migrate();
    spdlog::info("Database opened at {} (schema v{})", path_, schema_version());
}

Database::~Database()
{
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::exec(const std::string& sql)
{
    char* error_message = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_message);
    if (rc != SQLITE_OK) {
        std::string message = error_message != nullptr ? error_message : "sqlite3_exec failed";
        sqlite3_free(error_message);
        throw DatabaseError(message);
    }
}

int Database::schema_version() const
{
    return read_schema_version();
}

int Database::read_schema_version() const
{
    sqlite3_stmt* stmt = nullptr;
    check_sqlite(
        sqlite3_prepare_v2(db_, "SELECT version FROM schema_version LIMIT 1", -1, &stmt, nullptr),
        db_,
        "prepare schema_version");

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
}

void Database::write_schema_version(int version)
{
    if (read_schema_version() == 0) {
        exec("INSERT INTO schema_version(version) VALUES (" + std::to_string(version) + ")");
    } else {
        exec("UPDATE schema_version SET version = " + std::to_string(version));
    }
}

void Database::apply_migration(int version)
{
    if (version == 1) {
        exec(R"SQL(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL COLLATE NOCASE UNIQUE,
                password_hash TEXT NOT NULL,
                chips INTEGER NOT NULL DEFAULT 1000 CHECK (chips >= 0),
                created_at TEXT NOT NULL DEFAULT (datetime('now'))
            );
        )SQL");
        spdlog::info("Applied database migration v1 (users table)");
        return;
    }

    if (version == 2) {
        exec("PRAGMA journal_mode=WAL");
        exec(R"SQL(
            ALTER TABLE users ADD COLUMN updated_at TEXT NOT NULL DEFAULT (datetime('now'));
        )SQL");
        spdlog::info("Applied database migration v2 (WAL mode, users.updated_at)");
    }
}

void Database::migrate()
{
    exec(R"SQL(
        CREATE TABLE IF NOT EXISTS schema_version (
            version INTEGER NOT NULL
        );
    )SQL");

    int version = read_schema_version();
    while (version < kLatestSchemaVersion) {
        ++version;
        apply_migration(version);
        write_schema_version(version);
    }
}

} // namespace poker::server
