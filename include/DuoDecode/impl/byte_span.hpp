#pragma once
#include <cstddef>

namespace dd::impl {
    struct byte_span {
        std::byte* ptr;
        std::size_t size;
    };
}