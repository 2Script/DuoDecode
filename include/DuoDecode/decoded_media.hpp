#pragma once
#include <cstddef>
#include <cstdint>
#include <libavutil/pixfmt.h>
#include "DuoDecode/av_types.hpp"

namespace dd {
    struct decoded_video {
        av_vector<std::byte> data;
        std::uint32_t width;
        std::uint32_t height;
        AVPixelFormat format;
    };

    struct decoded_audio {
        av_vector<std::byte> data;
        std::int32_t samples;
        AVSampleFormat format;
    };
    
    
    struct decoded_media {
        decoded_video video;
        decoded_audio audio;
    };
}