#include "poker/packet_framing.h"

#include <cstring>
#include <stdexcept>

namespace poker::network {

namespace {

uint32_t to_big_endian(uint32_t value)
{
    return ((value & 0x000000FFu) << 24) | ((value & 0x0000FF00u) << 8) | ((value & 0x00FF0000u) >> 8)
        | ((value & 0xFF000000u) >> 24);
}

uint32_t from_big_endian(uint32_t value)
{
    return to_big_endian(value);
}

} // namespace

std::vector<uint8_t> encode_packet(std::string_view json_body)
{
    if (json_body.size() > MAX_PACKET_SIZE) {
        throw std::length_error("JSON body exceeds maximum packet size");
    }

    const uint32_t length = static_cast<uint32_t>(json_body.size());
    const uint32_t network_length = to_big_endian(length);

    std::vector<uint8_t> packet;
    packet.reserve(sizeof(network_length) + json_body.size());

    const auto* length_bytes = reinterpret_cast<const uint8_t*>(&network_length);
    packet.insert(packet.end(), length_bytes, length_bytes + sizeof(network_length));
    packet.insert(packet.end(), json_body.begin(), json_body.end());

    return packet;
}

bool decode_packet_length(const uint8_t* header_bytes, uint32_t& out_length)
{
    uint32_t network_length = 0;
    std::memcpy(&network_length, header_bytes, sizeof(network_length));
    out_length = from_big_endian(network_length);
    return is_valid_packet_length(out_length);
}

} // namespace poker::network
