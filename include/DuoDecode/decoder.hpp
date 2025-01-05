#pragma once
#include <filesystem>
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/filesystem.hpp"
#include "DuoDecode/impl/error.hpp"

namespace dd {
    class decoder {
    public:
        decoder() noexcept = default;
        decoder(filesystem::path_view input_file) noexcept;
        
    public:
        result<void> open() noexcept;
        result<void> decode_audio() noexcept;
        result<void> decode_video() noexcept;
        result<void> close() noexcept;

    private:
        std::filesystem::path input_path;
        std::byte* data;
        std::size_t data_size;
    };
}