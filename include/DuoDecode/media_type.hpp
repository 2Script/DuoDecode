#pragma once
#include <cstdint>
#include <type_traits>
extern "C" {
#include <libavutil/avutil.h>
}

namespace dd {
    struct media_type{
        enum flags : std::uint8_t {
            video = 0b1 << AVMEDIA_TYPE_VIDEO,
            audio = 0b1 << AVMEDIA_TYPE_AUDIO,
        };
    };
}


namespace dd {
    constexpr media_type::flags operator|(media_type::flags lhs, media_type::flags rhs) noexcept {
        using int_t = std::underlying_type_t<media_type::flags>;
        return static_cast<media_type::flags>(static_cast<int_t>(lhs) | static_cast<int_t>(rhs));
    }
}