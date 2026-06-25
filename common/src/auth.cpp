#include "poker/auth.h"

#include <algorithm>
#include <cctype>

namespace poker::auth {

std::string trim_copy(std::string value)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool is_valid_username(std::string_view username)
{
    if (username.size() < kMinUsernameLength || username.size() > kMaxUsernameLength) {
        return false;
    }

    for (unsigned char ch : username) {
        if (std::iscntrl(ch)) {
            return false;
        }
    }

    return true;
}

bool is_valid_password(std::string_view password)
{
    if (password.size() < kMinPasswordLength || password.size() > kMaxPasswordLength) {
        return false;
    }

    bool has_letter = false;
    bool has_digit = false;

    for (unsigned char ch : password) {
        if (std::iscntrl(ch)) {
            return false;
        }
        if (std::isalpha(ch)) {
            has_letter = true;
        } else if (std::isdigit(ch)) {
            has_digit = true;
        }
    }

    return has_letter && has_digit;
}

} // namespace poker::auth
