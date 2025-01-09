#include "DuoDecode/decoder.hpp"
#include <llfio/v2.0/file_handle.hpp>
#include <memory>
#include <result/type_traits.hpp>
#include <result/verify.h>
#include <type_traits>
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/error.hpp"
#include "DuoDecode/packet_handler.hpp"
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/mem.h>
#include <libavutil/pixfmt.h>
}


namespace dd {
    result<void> decoder::open(filesystem::path_view input_file) noexcept {
        //TODO: result<filesystem::mapped_file_handle> t = filesystem::mapped_file({}, input_path);
        {
        auto f = filesystem::mapped_file({}, input_file);
        if(!f.has_value()) return static_cast<dd::errc>(f.assume_error().value());
        file_handle = std::move(f).assume_value();
        data.ptr = file_handle.address();
        }

        {
        auto l = file_handle.maximum_extent();
        if(!l.has_value()) return static_cast<dd::errc>(l.assume_error().value());
        data.size = std::move(l).assume_value();
        }
        //TODO move above to file_decoder


        if(!(fmt_ctx = avformat_alloc_context()))
            return errc::not_enough_memory;

        io_buffer = {static_cast<std::byte*>(av_malloc(io_buffer_size)), io_buffer_size};
        if(!io_buffer.ptr)
            return errc::not_enough_memory;

        if(!(io_ctx = avio_alloc_context(reinterpret_cast<unsigned char*>(io_buffer.ptr), io_buffer.size, 0, &data, &packet_handler::read, &packet_handler::write, nullptr)))
            return errc::not_enough_memory;
        fmt_ctx->pb = io_ctx;

        if(!(pkt = av_packet_alloc()))
            return errc::not_enough_memory;

        int ret = 0;
        if((ret = avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr)) < 0)
            return static_cast<errc>(-ret);

        if((ret = avformat_find_stream_info(fmt_ctx, nullptr)) < 0)
            return static_cast<errc>(-ret);

        return {};
    }
}

namespace dd {
    result<void> decoder::decode_audio() noexcept {
        return {};
    }
}

namespace dd {
    result<decoded_video> decoder::decode_video(AVHWDeviceType device_type) noexcept {
        int ret = 0, video_stream_idx = 0;
        AVCodec const* decoder = nullptr;
        AVStream* video_stream = nullptr;
        
        if((ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0)) < 0)
            return static_cast<errc>(-ret);
        video_stream_idx = ret;

        if(!(dec_ctx = avcodec_alloc_context3(decoder)))
            return errc::not_enough_memory;
        
        video_stream = fmt_ctx->streams[video_stream_idx];
        if((ret = avcodec_parameters_to_context(dec_ctx, video_stream->codecpar)) < 0)
            return static_cast<errc>(-ret);


        if(device_type != AV_HWDEVICE_TYPE_NONE)
            RESULT_VERIFY(create_hw_context(device_type, decoder));

        if((ret = avcodec_open2(dec_ctx, decoder, nullptr)) < 0)
            return static_cast<errc>(-ret);

        av_vector<std::byte> ret_buffer;
        std::unique_ptr<AVPixelFormat> format;
        std::unique_ptr<int> width, height;

        frame frame;
        if(!(frame = av_frame_alloc())) {
            ret = -static_cast<int>(errc::not_enough_memory);
            goto unref;
        }

    read_frame:
        while(true) {
            if((ret = av_read_frame(fmt_ctx, pkt)) < 0) goto flush;
            if(video_stream_idx == pkt->stream_index) break;
            av_packet_unref(pkt);
        }

        if((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) goto unref;

        while(true) {
            if((ret = avcodec_receive_frame(dec_ctx, frame)) < 0)
                goto unref;

            if(!format || !width || !height) {
                format = std::make_unique<AVPixelFormat>(static_cast<AVPixelFormat>(frame->format));
                width  = std::make_unique<int>(frame->width);
                height = std::make_unique<int>(frame->height);
            }
            else if (std::tie(*format, *width, *height) != std::tie(frame->format, frame->width, frame->height))
                return errc::inconsistent_video_format;

            ret_buffer.clear();
            if((ret = av_image_get_buffer_size(*format, *width, *height, 1)) < 0)
                goto unref;
            ret_buffer.resize(static_cast<std::size_t>(ret));

            if((ret = av_image_copy_to_buffer(reinterpret_cast<std::uint8_t*>(ret_buffer.data()), ret_buffer.size(), frame->data, frame->linesize, *format, *width, *height, 1)) < 0)
                goto unref;
        }

    unref:
        av_packet_unref(pkt);
    flush:
        avcodec_send_packet(dec_ctx, nullptr);
        //if(ret != 0) return static_cast<errc>(-ret);
        //return std::move(ret_buffer);
        switch(-ret) {
        case 0:
        case static_cast<int>(errc::end_of_file):
            return decoded_video{std::move(ret_buffer), static_cast<std::uint32_t>(*width), static_cast<std::uint32_t>(*height), *format};
        case static_cast<int>(errc::resource_unavailable_try_again):
            goto read_frame;
        default:
            return static_cast<errc>(-ret);
        }
    }
}

namespace dd {
    result<void> decoder::close() noexcept {
        return static_cast<result<void>>(file_handle.close());
    }
}


namespace dd {
    result<void> decoder::create_hw_context(AVHWDeviceType device_type, AVCodec const* decoder) noexcept {
        AVPixelFormat desired_pixel_format = AV_PIX_FMT_NONE;

        for(int i = 0;; ++i) {
            AVCodecHWConfig const* config = avcodec_get_hw_config(decoder, i);
            if(!config) {
                desired_pixel_format = hw_pixel_formats[device_type];
                break;
            }
            if(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == device_type){
                desired_pixel_format = config->pix_fmt;
                break;
            }
        }

        dec_ctx->opaque = &desired_pixel_format;
        dec_ctx->get_format = [](AVCodecContext* ctx, AVPixelFormat const* pixel_formats) noexcept -> AVPixelFormat {
            for(AVPixelFormat const* pix_fmts = pixel_formats; *pix_fmts != AV_PIX_FMT_NONE; ++pix_fmts) 
                if(*pix_fmts == *static_cast<AVPixelFormat*>(ctx->opaque))
                    return *pix_fmts;
            return AV_PIX_FMT_NONE; 
        };

        
        if (int ret = av_hwdevice_ctx_create(&device_ctx, device_type, nullptr, nullptr, 0); ret < 0)
            return static_cast<errc>(-ret);
        dec_ctx->hw_device_ctx = av_buffer_ref(device_ctx);

        return {};
    }
}


namespace dd {
    std::array<bool, decoder::device_type_count> decoder::device_types() const noexcept {
        std::array<bool, decoder::device_type_count> ret = {};
        ret[AV_HWDEVICE_TYPE_NONE] = true;

        AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            ret[type] = true;

        return ret;
    }
}