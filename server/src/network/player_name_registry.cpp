#include "network/player_name_registry.h"

namespace poker::server {

bool PlayerNameRegistry::try_register(const std::string& name)
{
    return names_.insert(name).second;
}

void PlayerNameRegistry::unregister(const std::string& name)
{
    names_.erase(name);
}

bool PlayerNameRegistry::is_taken(const std::string& name) const
{
    return names_.count(name) > 0;
}

} // namespace poker::server
