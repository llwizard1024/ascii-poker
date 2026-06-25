#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace poker::auth {

inline constexpr std::size_t kMinUsernameLength = 1;
inline constexpr std::size_t kMaxUsernameLength = 32;
inline constexpr std::size_t kMinPasswordLength = 4;
inline constexpr std::size_t kMaxPasswordLength = 128;

std::string trim_copy(std::string value);
bool is_valid_username(std::string_view username);
bool is_valid_password(std::string_view password);

} // namespace poker::auth
