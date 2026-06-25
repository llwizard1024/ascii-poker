#include "storage/password_hash.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace poker::server {

namespace {

constexpr int kIterations = 100000;
constexpr size_t kSaltBytes = 16;
constexpr size_t kHashBytes = 32;

std::string to_hex(const unsigned char* data, size_t size)
{
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < size; ++i) {
        ss << std::setw(2) << static_cast<unsigned>(data[i]);
    }
    return ss.str();
}

bool from_hex(std::string_view hex, std::vector<unsigned char>& out)
{
    if (hex.size() % 2 != 0) {
        return false;
    }

    out.resize(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        const auto byte = hex.substr(i * 2, 2);
        unsigned value = 0;
        std::istringstream ss { std::string(byte) };
        ss >> std::hex >> value;
        if (ss.fail()) {
            return false;
        }
        out[i] = static_cast<unsigned char>(value);
    }
    return true;
}

std::array<unsigned char, kHashBytes> derive_key(
    std::string_view password,
    const unsigned char* salt,
    size_t salt_size)
{
    std::array<unsigned char, kHashBytes> out {};
    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            static_cast<int>(password.size()),
            salt,
            static_cast<int>(salt_size),
            kIterations,
            EVP_sha256(),
            static_cast<int>(out.size()),
            out.data())
        != 1) {
        throw std::runtime_error("PBKDF2 failed");
    }
    return out;
}

} // namespace

std::string PasswordHasher::hash(std::string_view password)
{
    std::array<unsigned char, kSaltBytes> salt {};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }

    const auto key = derive_key(password, salt.data(), salt.size());
    std::ostringstream ss;
    ss << "pbkdf2-sha256$" << kIterations << '$' << to_hex(salt.data(), salt.size()) << '$'
       << to_hex(key.data(), key.size());
    return ss.str();
}

bool PasswordHasher::verify(std::string_view password, std::string_view stored_hash)
{
    const std::string value(stored_hash);
    const auto first = value.find('$');
    const auto second = value.find('$', first + 1);
    const auto third = value.find('$', second + 1);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos) {
        return false;
    }

    if (value.substr(0, first) != "pbkdf2-sha256") {
        return false;
    }

    const int iterations = std::stoi(value.substr(first + 1, second - first - 1));
    const std::string salt_hex = value.substr(second + 1, third - second - 1);
    const std::string hash_hex = value.substr(third + 1);

    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected_hash;
    if (!from_hex(salt_hex, salt) || !from_hex(hash_hex, expected_hash)
        || expected_hash.size() != kHashBytes) {
        return false;
    }

    std::array<unsigned char, kHashBytes> actual {};
    if (PKCS5_PBKDF2_HMAC(
            password.data(),
            static_cast<int>(password.size()),
            salt.data(),
            static_cast<int>(salt.size()),
            iterations,
            EVP_sha256(),
            static_cast<int>(actual.size()),
            actual.data())
        != 1) {
        return false;
    }

    return CRYPTO_memcmp(actual.data(), expected_hash.data(), actual.size()) == 0;
}

} // namespace poker::server
