#pragma once

#include <string>
#include <unordered_set>

namespace poker::server {

class PlayerNameRegistry {
public:
    bool try_register(const std::string& name);
    void unregister(const std::string& name);
    bool is_taken(const std::string& name) const;

private:
    std::unordered_set<std::string> names_;
};

} // namespace poker::server
