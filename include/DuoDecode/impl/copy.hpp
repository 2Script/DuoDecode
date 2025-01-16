#pragma once
#include "DuoDecode/decoded_media.hpp"
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/error.hpp"

namespace dd::impl {
    void copy_audio(decoded_audio& dst, frame& src, std::size_t ch) noexcept;

    //Exactly the same as `av_image_copy_to_buffer` except that it uses zsimd memcpy instead
    result<void> copy_video(decoded_video& dst, frame& src) noexcept;
}