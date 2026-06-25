#include "storage/database.h"

#include <catch_amalgamated.hpp>

#include <chrono>
#include <filesystem>
#include <string>

using namespace poker::server;

namespace {

std::string unique_db_path()
{
    return (std::filesystem::temp_directory_path()
        / ("ascii_poker_db_test_" + std::to_string(
               std::chrono::steady_clock::now().time_since_epoch().count())
            + ".db"))
        .string();
}

bool column_exists(Database& database, const std::string& table, const std::string& column)
{
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = "PRAGMA table_info(" + table + ")";
    if (sqlite3_prepare_v2(database.handle(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name != nullptr && column == name) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}

} // namespace

TEST_CASE("Fresh database applies all migrations", "[storage]")
{
    const std::string path = unique_db_path();
    Database database(path);

    REQUIRE(database.schema_version() == 2);
    REQUIRE(column_exists(database, "users", "updated_at"));
}

TEST_CASE("Opening an existing database is idempotent", "[storage]")
{
    const std::string path = unique_db_path();
    {
        Database database(path);
        REQUIRE(database.schema_version() == 2);
    }
    {
        Database database(path);
        REQUIRE(database.schema_version() == 2);
    }
}

TEST_CASE("Database migrates from schema version 1 to 2", "[storage]")
{
    const std::string path = unique_db_path();
    {
        sqlite3* raw_db = nullptr;
        REQUIRE(sqlite3_open(path.c_str(), &raw_db) == SQLITE_OK);
        REQUIRE(sqlite3_exec(
            raw_db,
            R"SQL(
                CREATE TABLE schema_version (version INTEGER NOT NULL);
                INSERT INTO schema_version(version) VALUES (1);
                CREATE TABLE users (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    username TEXT NOT NULL COLLATE NOCASE UNIQUE,
                    password_hash TEXT NOT NULL,
                    chips INTEGER NOT NULL DEFAULT 1000 CHECK (chips >= 0),
                    created_at TEXT NOT NULL DEFAULT (datetime('now'))
                );
            )SQL",
            nullptr,
            nullptr,
            nullptr)
            == SQLITE_OK);
        sqlite3_close(raw_db);
    }

    Database database(path);
    REQUIRE(database.schema_version() == 2);
    REQUIRE(column_exists(database, "users", "updated_at"));
}
