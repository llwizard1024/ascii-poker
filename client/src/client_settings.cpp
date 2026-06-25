#include "client_settings.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace poker::client {

namespace {

    std::string settings_path()
    {
        if (const char* home = std::getenv("HOME")) {
            return std::string(home) + "/.ascii-poker.conf";
        }
        return ".ascii-poker.conf";
    }

    void trim(std::string& value)
    {
        const auto start = value.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            value.clear();
            return;
        }
        const auto end = value.find_last_not_of(" \t\r\n");
        value = value.substr(start, end - start + 1);
    }

} // namespace

ClientSettings load_client_settings()
{
    ClientSettings settings;
    std::ifstream file(settings_path());
    if (!file) {
        return settings;
    }

    std::string line;
    while (std::getline(file, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        trim(key);
        trim(value);

        if (key == "player_name") {
            settings.player_name = value;
        } else if (key == "host") {
            settings.host = value;
        } else if (key == "port") {
            settings.port = value;
        } else if (key == "language") {
            settings.language = value;
        }
    }

    return settings;
}

void save_client_settings(const ClientSettings& settings)
{
    std::ofstream file(settings_path());
    if (!file) {
        return;
    }

    if (!settings.player_name.empty()) {
        file << "player_name=" << settings.player_name << '\n';
    }
    file << "host=" << settings.host << '\n';
    file << "port=" << settings.port << '\n';
    file << "language=" << settings.language << '\n';
}

} // namespace poker::client
