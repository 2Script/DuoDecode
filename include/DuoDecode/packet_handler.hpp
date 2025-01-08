#pragma once
#include <cstdint>

namespace dd {
    namespace packet_handler{
        int read(void* src_data, std::uint8_t* dst, int dst_size);
        int write(void* dst_data, const std::uint8_t* src, int src_size);
    }
}