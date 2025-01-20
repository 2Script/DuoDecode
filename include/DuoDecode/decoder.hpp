#pragma once
#include <iterator>
#include <optional>
#include <type_traits>
#include "DuoDecode/decoded_media.hpp"
#include "DuoDecode/device_info.hpp"
#include "DuoDecode/error.hpp"
#include "DuoDecode/impl/to_addr.hpp"
#include "DuoDecode/media_type.hpp"
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/impl/byte_span.hpp"
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}


namespace dd {
    class decoder {
    public:
        constexpr decoder() noexcept = default;
        template<typename It>
        constexpr decoder(It first, std::size_t count) noexcept : data{reinterpret_cast<std::byte const*>(impl::to_addr(first)), count} {}

        template<typename It, typename End, std::enable_if_t<!std::is_convertible_v<End, std::size_t>, bool> = true>
        constexpr decoder(It first, End last) noexcept : decoder(first, last - first) {}
        
        template<typename T, std::size_t N>
        constexpr decoder(T (&arr)[N]) noexcept : decoder(std::data(arr), N) {}
        template<typename T, std::size_t N>
        constexpr decoder(std::array<T, N>& arr) noexcept : decoder(std::data(arr), N) {}
        template<typename T, std::size_t N>
        constexpr decoder(std::array<T, N> const& arr) noexcept : decoder(std::data(arr), N) {}

    public:
        result<decoded_media> decode(AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE, media_type::flags media_types = (media_type::video | media_type::audio), result<void>(&device_create_fn)(AVHWDeviceContext*) = generic_create_device) noexcept;
        result<decoded_audio> decode_audio_only() noexcept;
        result<decoded_video> decode_video_only(AVHWDeviceType hw_device_type = AV_HWDEVICE_TYPE_NONE) noexcept;

        result<format_context> fmt_context(io_context* io_ctx_ref = nullptr) noexcept;

    public:
        constexpr static std::size_t device_type_count = AV_HWDEVICE_TYPE_D3D12VA + 1;
        std::array<bool, device_type_count> device_types() const noexcept;
        //result<std::pair<std::vector<device_info>, std::optional<std::size_t>>> devices() noexcept;

    private:
        result<int> alloc_dec_context(AVCodec const*& decoder, codec_context& dec_ctx) const noexcept;
        result<void> open_dec_context(AVCodec const*& decoder, codec_context& dec_ctx) const noexcept;
        result<void> create_hw_context(AVCodec const* decoder, AVHWDeviceType device_type, result<void>(&device_create_fn)(AVHWDeviceContext*)) noexcept;

        static result<void> generic_create_device(AVHWDeviceContext* dev_ctx) noexcept;


    private:
        constexpr static std::size_t io_buffer_size = 0x1000;

        //copy the data ptr and size to input, since libav modifies it (we need to perserve the original)
        const impl::byte_span data{};
        //impl::byte_span input{};
        
        //std::byte* io_buffer_ptr = nullptr;
        //io_context io_ctx;
        format_context fmt_ctx;
        codec_context video_dec_ctx, audio_dec_ctx;
        buffer_ref device_ctx;
        //packet pkt;

        constexpr static std::array<AVPixelFormat, device_type_count> hw_pixel_formats = {
            AV_PIX_FMT_NONE,         //AV_HWDEVICE_TYPE_NONE
            AV_PIX_FMT_VDPAU,        //AV_HWDEVICE_TYPE_VDPAU
            AV_PIX_FMT_CUDA,         //AV_HWDEVICE_TYPE_CUDA
            AV_PIX_FMT_VAAPI,        //AV_HWDEVICE_TYPE_VAAPI
            AV_PIX_FMT_DXVA2_VLD,    //AV_HWDEVICE_TYPE_DXVA2
            AV_PIX_FMT_QSV,          //AV_HWDEVICE_TYPE_QSV
            AV_PIX_FMT_VIDEOTOOLBOX, //AV_HWDEVICE_TYPE_VIDEOTOOLBOX
            AV_PIX_FMT_D3D11,        //AV_HWDEVICE_TYPE_D3D11VA
            AV_PIX_FMT_DRM_PRIME,    //AV_HWDEVICE_TYPE_DRM
            AV_PIX_FMT_OPENCL,       //AV_HWDEVICE_TYPE_OPENCL
            AV_PIX_FMT_MEDIACODEC,   //AV_HWDEVICE_TYPE_MEDIACODEC
            AV_PIX_FMT_VULKAN,       //AV_HWDEVICE_TYPE_VULKAN
            AV_PIX_FMT_D3D12,        //AV_HWDEVICE_TYPE_D3D12VA
        };
    };
}
