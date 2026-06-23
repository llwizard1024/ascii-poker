#pragma once

#include "poker/network_limits.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace poker::network {

inline bool is_valid_packet_length(uint32_t length)
{
    return length > 0 && length <= MAX_PACKET_SIZE;
}

std::vector<uint8_t> encode_packet(std::string_view json_body);

bool decode_packet_length(const uint8_t* header_bytes, uint32_t& out_length);

} // namespace poker::network
