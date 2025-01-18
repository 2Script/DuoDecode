#include "DuoDecode/decoder.hpp"
#include <climits>
#include <iterator>
#include <memory>
#include <result/type_traits.hpp>
#include <result/verify.h>
#include <type_traits>
#include <zstring.hpp>
#include "DuoDecode/av_types.hpp"
#include "DuoDecode/decoded_media.hpp"
#include "DuoDecode/error.hpp"
#include "DuoDecode/impl/byte_span.hpp"
#include "DuoDecode/impl/copy.hpp"
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
}


namespace dd {
    result<decoded_media> decoder::decode(AVHWDeviceType hw_device_type, media_type::flags media_types) noexcept {
        if(media_types == 0) return decoded_media{};
        RESULT_VERIFY(create_fmt_context());

        int video_stream_idx = 0, audio_stream_idx = 0;
        
        if(media_types & media_type::video)
            RESULT_TRY_COPY(video_stream_idx, create_dec_context(AVMEDIA_TYPE_VIDEO, hw_device_type));

        if(media_types & media_type::audio)
            RESULT_TRY_COPY(audio_stream_idx, create_dec_context(AVMEDIA_TYPE_AUDIO, hw_device_type));


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
    result<void> decoder::create_fmt_context() noexcept {
        if(data.size == 0) return errc::invalid_argument;

        if(!(fmt_ctx = avformat_alloc_context()))
            return errc::not_enough_memory;
        
        io_buffer_ptr = static_cast<std::byte*>(av_malloc(io_buffer_size));
        if(!io_buffer_ptr) return errc::not_enough_memory;

        input = data; //copy the data ptr and size since libav modifies it (we need to perserve the original)
        if(!(io_ctx = avio_alloc_context(reinterpret_cast<unsigned char*>(io_buffer_ptr), io_buffer_size, 0, &input, &packet_handler::read, nullptr, nullptr)))
            return errc::not_enough_memory; 
        fmt_ctx->pb = io_ctx;


        int err = 0;
        if((err = -avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr)) > 0)
            return static_cast<errc>(err);

        if((err = -avformat_find_stream_info(fmt_ctx, nullptr)) > 0)
            return static_cast<errc>(err);

        return {};
    }
}

namespace dd {
    result<int> decoder::create_dec_context(AVMediaType media_type, AVHWDeviceType device_type) noexcept {
        AVCodec const* decoder = nullptr; 
        
        int stream_idx = 0;
        if((stream_idx = av_find_best_stream(fmt_ctx, media_type, -1, -1, &decoder, 0)) < 0)
            return static_cast<errc>(-stream_idx);
        codec_context& dec_ctx = (media_type == AVMEDIA_TYPE_VIDEO ? video_dec_ctx : audio_dec_ctx);

        if(!(dec_ctx = avcodec_alloc_context3(decoder)))
            return errc::not_enough_memory;
        

        int err = 0;
        AVStream* video_stream = fmt_ctx->streams[stream_idx];
        if((err = -avcodec_parameters_to_context(dec_ctx, video_stream->codecpar)) > 0)
            return static_cast<errc>(err);

        if(media_type == AVMEDIA_TYPE_VIDEO && device_type != AV_HWDEVICE_TYPE_NONE)
            RESULT_VERIFY(create_hw_context(device_type, decoder));

        if((err = -avcodec_open2(dec_ctx, decoder, nullptr)) > 0)
            return static_cast<errc>(err);
        return stream_idx;
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

        video_dec_ctx->opaque = &desired_pixel_format;
        video_dec_ctx->get_format = [](AVCodecContext* ctx, AVPixelFormat const* pixel_formats) noexcept -> AVPixelFormat {
            for(AVPixelFormat const* pix_fmts = pixel_formats; *pix_fmts != AV_PIX_FMT_NONE; ++pix_fmts) 
                if(*pix_fmts == *static_cast<AVPixelFormat*>(ctx->opaque))
                    return *pix_fmts;
            return AV_PIX_FMT_NONE; 
        };

        
        if (int err = -av_hwdevice_ctx_create(&device_ctx, device_type, nullptr, nullptr, 0); err > 0)
            return static_cast<errc>(err);
        video_dec_ctx->hw_device_ctx = av_buffer_ref(device_ctx);

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
    

    //std::pair<std::vector<device_info>, std::size_t> decoder::devices() const noexcept {
    //    RESULT_VERIFY(create_fmt_context());
    //    
    //}
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
