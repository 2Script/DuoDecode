#include "DuoDecode/decoder.hpp"
#include <climits>
#include <memory>
#include <optional>
#include <result/type_traits.hpp>
#include <result/verify.h>
#include <type_traits>
#include <zstring.hpp>
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/decoded_media.hpp"
#include "DuoDecode/error.hpp"
#include "DuoDecode/impl/byte_span.hpp"
#include "DuoDecode/impl/copy.hpp"
#include "DuoDecode/impl/device_context.hpp"
#include "DuoDecode/media_type.hpp"
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
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavdevice/avdevice.h>
}


namespace dd {
    result<decoded_media> decoder::decode(AVHWDeviceType hw_device_type, media_type::flags media_types, result<void>(&device_create_fn)(AVHWDeviceContext*)) noexcept {
        if(media_types == 0) return decoded_media{};


        std::byte* io_buffer_ptr = static_cast<std::byte*>(av_malloc(io_buffer_size));
        if(!io_buffer_ptr) return errc::not_enough_memory;

        io_context io_ctx;
        impl::byte_span input = data; //copy the data ptr and size since libav modifies it (we need to perserve the original)
        if(!(io_ctx = avio_alloc_context(reinterpret_cast<unsigned char*>(io_buffer_ptr), io_buffer_size, 0, &input, &packet_handler::read, nullptr, nullptr)))
            return errc::not_enough_memory; 
        RESULT_TRY_MOVE(fmt_ctx, fmt_context(std::addressof(io_ctx)));


        int video_stream_idx = 0, audio_stream_idx = 0;
        
        {
        AVCodec const* decoder = nullptr;
        if(media_types & media_type::video) {
            RESULT_TRY_COPY(video_stream_idx, alloc_dec_context(decoder, video_dec_ctx));
            RESULT_VERIFY(create_hw_context(decoder, hw_device_type, device_create_fn));
            RESULT_VERIFY(open_dec_context(decoder, video_dec_ctx));
        }

        if(media_types & media_type::audio) {
            RESULT_TRY_COPY(audio_stream_idx, alloc_dec_context(decoder, audio_dec_ctx));
            RESULT_VERIFY(open_dec_context(decoder, audio_dec_ctx));
        }
        }

        decoded_media ret = {};
        codec_context* curr_context = nullptr;
        int err = 0;

        frame curr_frame;
        if(!(curr_frame = av_frame_alloc())) 
            return errc::not_enough_memory;

        packet pkt;
        if(!(pkt = av_packet_alloc()))
            return errc::not_enough_memory;

    read_frame:
        while(true) {
            if((err = -av_read_frame(fmt_ctx, pkt)) > 0) goto handle_eof;
            if(video_stream_idx == pkt->stream_index && media_types & media_type::video) { curr_context = std::addressof(video_dec_ctx); break; }
            if(audio_stream_idx == pkt->stream_index && media_types & media_type::audio) { curr_context = std::addressof(audio_dec_ctx); break; }
            av_packet_unref(pkt);
        }

        err = -avcodec_send_packet(*curr_context, pkt);
        if(err > 0 && err != static_cast<std::int32_t>(errc::resource_unavailable_try_again)) goto handle_eof;

        while(true) {
            switch((err = -avcodec_receive_frame(*curr_context, curr_frame))) {
            case 0: 
                break;
            case static_cast<std::int32_t>(errc::resource_unavailable_try_again): 
                av_packet_unref(pkt);
                goto read_frame;
            default: 
                goto handle_eof;
            }

            decoded_video& ret_video = ret.video;
            decoded_audio& ret_audio = ret.audio;


            if(curr_context != std::addressof(video_dec_ctx)) goto output_audio;

            if(ret_video.format == AV_PIX_FMT_NONE) {
                ret_video.format = static_cast<AVPixelFormat>(curr_frame->format);
                ret_video.width  = curr_frame->width;
                ret_video.height = curr_frame->height;
            }
            else if (std::tie(ret_video.format, ret_video.width, ret_video.height) != std::tie(curr_frame->format, curr_frame->width, curr_frame->height))
                return errc::inconsistent_video_format;

            if(auto r = impl::copy_video(ret_video, curr_frame); !r.has_value()) {
                err = static_cast<std::int32_t>(r.error());
                goto handle_eof;
            }
        

        output_audio:
            if(curr_context != std::addressof(audio_dec_ctx)) continue;

            if(ret_audio.format == AV_SAMPLE_FMT_NONE) {
                ret_audio.format       = static_cast<AVSampleFormat>(curr_frame->format);
                ret_audio.num_channels = curr_frame->ch_layout.nb_channels;
                ret_audio.num_samples  = 0;
                ret_audio.sample_rate  = curr_frame->sample_rate;
                ret_audio.bit_depth    = av_get_bytes_per_sample(ret_audio.format) * CHAR_BIT;
                ret_audio.planar       = av_sample_fmt_is_planar(ret_audio.format);
            }
            ret_audio.num_samples += curr_frame->nb_samples;

            switch(curr_frame->ch_layout.order) {
            case AV_CHANNEL_ORDER_NATIVE:
                for(std::size_t ch = 0; ch < max_audio_channels; ++ch)
                    if(curr_frame->ch_layout.u.mask & (1ULL << ch))
                        impl::copy_audio(ret_audio, curr_frame, ch);
                break;

            case AV_CHANNEL_ORDER_CUSTOM:
                for(std::size_t i = 0; i < ret_audio.num_channels; ++i) {
                    const std::uint32_t ch = curr_frame->ch_layout.u.map[i].id;
                    if(ch < max_audio_channels)
                        impl::copy_audio(ret_audio, curr_frame, ch);
                }
                break;

            default:
                return errc::invalid_audio_channel_order;
            }

        }

    handle_eof:
        switch(err) {
        case 0:
        case static_cast<int>(errc::end_of_file):
            return ret;
        default:
            return static_cast<errc>(err);
        }
    }
}

namespace dd {
    result<format_context> decoder::fmt_context(io_context* io_ctx_ref) noexcept {
        if(data.size == 0) return errc::invalid_argument;

        format_context ret;
        if(!(ret = avformat_alloc_context()))
            return errc::not_enough_memory;
        
        if(io_ctx_ref) ret->pb = *io_ctx_ref;

        int err = 0;
        if((err = -avformat_open_input(&ret, nullptr, nullptr, nullptr)) > 0)
            return static_cast<errc>(err);

        if((err = -avformat_find_stream_info(ret, nullptr)) > 0)
            return static_cast<errc>(err);

        return std::move(ret);
    }
}



namespace dd {
    result<int> decoder::alloc_dec_context(AVCodec const*& decoder, codec_context& dec_ctx) const noexcept {
        int stream_idx = 0;
        if((stream_idx = av_find_best_stream(fmt_ctx, static_cast<AVMediaType>(std::addressof(dec_ctx) == std::addressof(audio_dec_ctx)), -1, -1, &decoder, 0)) < 0)
            return static_cast<errc>(-stream_idx);

        if(!(dec_ctx = avcodec_alloc_context3(decoder)))
            return errc::not_enough_memory;
        

        int err = 0;
        AVStream* stream = fmt_ctx->streams[stream_idx];
        if((err = -avcodec_parameters_to_context(dec_ctx, stream->codecpar)) > 0)
            return static_cast<errc>(err);

        return stream_idx;
    }
    
    result<void> decoder::open_dec_context(AVCodec const*& decoder, codec_context& dec_ctx) const noexcept {
        if(int err = -avcodec_open2(dec_ctx, decoder, nullptr); err > 0)
            return static_cast<errc>(err);
        return {};
    }


    result<void> decoder::create_hw_context(AVCodec const* decoder, AVHWDeviceType device_type, result<void>(&device_create_fn)(AVHWDeviceContext*)) noexcept {
        if(device_type == AV_HWDEVICE_TYPE_NONE) return {};
        
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

        video_dec_ctx->opaque = &desired_pixel_format;
        video_dec_ctx->get_format = [](AVCodecContext* ctx, AVPixelFormat const* pixel_formats) noexcept -> AVPixelFormat {
            for(AVPixelFormat const* pix_fmts = pixel_formats; *pix_fmts != AV_PIX_FMT_NONE; ++pix_fmts) 
                if(*pix_fmts == *static_cast<AVPixelFormat*>(ctx->opaque))
                    return *pix_fmts;
            return AV_PIX_FMT_NONE; 
        };


        if(!(device_ctx = av_hwdevice_ctx_alloc(device_type)))
            return errc::not_enough_memory;

        RESULT_VERIFY(device_create_fn(reinterpret_cast<AVHWDeviceContext*>(device_ctx->data)));

        if (int err = -av_hwdevice_ctx_init(device_ctx); err > 0)
            return static_cast<errc>(err);

        video_dec_ctx->hw_device_ctx = av_buffer_ref(device_ctx);

        return {};
    }
}


namespace dd {
    //Does the same as calling `av_hwdevice_ctx_create` without alloc and init
    result<void> decoder::generic_create_device(AVHWDeviceContext* dev_ctx) noexcept {
        impl::device_context* device_ctx = reinterpret_cast<impl::device_context*>(dev_ctx);

        if (!device_ctx->hw_type->device_create) return errc::function_not_supported;
  
        if(int err = -device_ctx->hw_type->device_create(&(device_ctx->p), nullptr, nullptr, 0); err > 0)
            return static_cast<errc>(err);
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
    

    /*result<std::pair<std::vector<device_info>, std::optional<std::size_t>>> decoder::devices() const noexcept {
        RESULT_VERIFY(create_fmt_context());
        using ret_type = std::pair<std::vector<device_info>, std::optional<std::size_t>>;
        
        device_info_list device_list;
        int err = 0;

        if((err = -avdevice_list_devices(fmt_ctx, &device_list)) > 0)
            return static_cast<errc>(err);

        if(device_list->nb_devices < 0) return errc::no_such_device;
        std::size_t num_devices = static_cast<std::size_t>(device_list->nb_devices);

        std::optional<std::size_t> default_device_idx = std::nullopt;
        if(device_list->default_device > 0) default_device_idx = static_cast<std::size_t>(device_list->default_device);

        std::vector<device_info> ret;
        ret.reserve(num_devices);
        for(std::size_t i = 0; i < num_devices; ++i) {
            AVDeviceInfo* device = device_list->devices[i];

            std::array<bool, AVMEDIA_TYPE_NB> has_media_types;
            if(device->nb_media_types > 0)
                for(std::size_t j = 0; j < static_cast<std::size_t>(device->nb_media_types); ++j)
                    has_media_types[device->media_types[j]] = true;
                 
            ret.push_back(device_info{device_list->devices[i]->device_name, device_list->devices[i]->device_description, has_media_types, default_device_idx.has_value() && *default_device_idx == i});
        }

        return result<ret_type>{std::in_place_type<ret_type>, std::move(ret), std::move(default_device_idx)};
    }*/
}


namespace dd {
    result<decoded_audio> decoder::decode_audio_only() noexcept {
        auto r = decode(AV_HWDEVICE_TYPE_NONE, media_type::audio);
        if(!r.has_value()) return r.error();
        return std::move(r)->audio;
    }

    result<decoded_video> decoder::decode_video_only(AVHWDeviceType hw_device_type) noexcept {
        auto r = decode(hw_device_type, media_type::video);
        if(!r.has_value()) return r.error();
        return std::move(r)->video;
    }
}
