#pragma once

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace poker::server {

class DatabaseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Database {
public:
    explicit Database(std::string path);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const { return db_; }
    const std::string& path() const { return path_; }

    void exec(const std::string& sql);
    int schema_version() const;

private:
    void migrate();
    int read_schema_version() const;
    void write_schema_version(int version);
    void apply_migration(int version);

    static constexpr int kLatestSchemaVersion = 2;

    std::string path_;
    sqlite3* db_ = nullptr;
};

std::string default_database_path();

} // namespace poker::server
