#pragma once
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <array>
extern "C" {
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}
#include "DuoDecode/av_types.hpp"

namespace dd {
    constexpr std::size_t max_audio_channels = AV_CHAN_TOP_BACK_RIGHT + 1;

    struct decoded_video {
        av_vector<std::byte> bytes;
        std::uint32_t width;
        std::uint32_t height;
        AVPixelFormat format = AV_PIX_FMT_NONE;
    };

    struct decoded_audio {
        std::array<av_vector<std::byte>, max_audio_channels> bytes;
        std::uint32_t num_samples;
        std::uint32_t num_channels;
        std::bitset<max_audio_channels> channel_mask;
        std::int32_t sample_rate;
        AVSampleFormat format = AV_SAMPLE_FMT_NONE;
        std::uint8_t bit_depth;
        bool planar;
    };
    
    
    struct decoded_media {
        decoded_video video;
        decoded_audio audio;
    };
}