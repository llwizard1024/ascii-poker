#include "poker/packet_framing.h"

#include <catch_amalgamated.hpp>

#include <cstring>
#include <string>

namespace {

uint32_t to_big_endian(uint32_t value)
{
    return ((value & 0x000000FFu) << 24) | ((value & 0x0000FF00u) << 8) | ((value & 0x00FF0000u) >> 8)
        | ((value & 0xFF000000u) >> 24);
}

} // namespace

using namespace poker::network;

TEST_CASE("encode_packet prepends big-endian length", "[packet_framing]")
{
    const std::string body = R"({"type":"list_rooms","data":{}})";
    const auto packet = encode_packet(body);

    REQUIRE(packet.size() == 4 + body.size());

    uint32_t network_length = 0;
    std::memcpy(&network_length, packet.data(), sizeof(network_length));

    REQUIRE(to_big_endian(network_length) == body.size());
    REQUIRE(std::string(packet.begin() + 4, packet.end()) == body);
}

TEST_CASE("decode_packet_length round-trip", "[packet_framing]")
{
    const std::string body = "hello";
    const auto packet = encode_packet(body);

    uint32_t length = 0;
    REQUIRE(decode_packet_length(packet.data(), length));
    REQUIRE(length == body.size());
}

TEST_CASE("decode_packet_length rejects invalid sizes", "[packet_framing]")
{
    uint32_t length = 0;

    uint32_t zero = to_big_endian(0);
    REQUIRE_FALSE(decode_packet_length(reinterpret_cast<const uint8_t*>(&zero), length));

    uint32_t too_large = to_big_endian(MAX_PACKET_SIZE + 1);
    REQUIRE_FALSE(decode_packet_length(reinterpret_cast<const uint8_t*>(&too_large), length));
}

TEST_CASE("is_valid_packet_length boundary values", "[packet_framing]")
{
    REQUIRE_FALSE(is_valid_packet_length(0));
    REQUIRE(is_valid_packet_length(1));
    REQUIRE(is_valid_packet_length(MAX_PACKET_SIZE));
    REQUIRE_FALSE(is_valid_packet_length(MAX_PACKET_SIZE + 1));
}

TEST_CASE("encode_packet rejects oversized body", "[packet_framing]")
{
    const std::string body(MAX_PACKET_SIZE + 1, 'x');
    REQUIRE_THROWS_AS(encode_packet(body), std::length_error);
}
