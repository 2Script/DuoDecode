#pragma once
#include <filesystem>
#include <libavutil/pixfmt.h>
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/filesystem.hpp"
#include "DuoDecode/decoded_media.hpp"
#include "DuoDecode/error.hpp"
#include "DuoDecode/impl/byte_span.hpp"

namespace dd {
    class decoder {
    public:
        decoder() noexcept = default;
        
    public:
        result<void> open(filesystem::path_view input_file) noexcept;
        result<void> decode_audio() noexcept;
        result<decoded_video> decode_video(AVHWDeviceType device_type) noexcept;
        result<void> close() noexcept;

    private:
        result<void> create_hw_context(AVHWDeviceType device_type, AVCodec const* decoder) noexcept;

    public:
        constexpr static std::size_t device_type_count = AV_HWDEVICE_TYPE_D3D12VA + 1;
        std::array<bool, device_type_count> device_types() const noexcept;

    private:
        constexpr static std::size_t io_buffer_size = 0x1000;

        filesystem::mapped_file_handle file_handle;
        impl::byte_span data = {}, io_buffer;
        

        format_context fmt_ctx;
        io_context io_ctx;
        codec_context dec_ctx;
        buffer_ref device_ctx;
        packet pkt;

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