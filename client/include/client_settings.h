#pragma once

#include <string>

namespace poker::client {

struct ClientSettings {
    std::string player_name;
    std::string host = "127.0.0.1";
    std::string port = "12345";
    std::string language = "en";
};

ClientSettings load_client_settings();
void save_client_settings(const ClientSettings& settings);

} // namespace poker::client
