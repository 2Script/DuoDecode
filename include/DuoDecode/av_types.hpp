#pragma once
#include <memory>
#include <vector>
#include "DuoDecode/impl/av_ptr.hpp"
#include "DuoDecode/impl/av_allocator.hpp"
extern "C" {
#include <libavutil/mem.h>
#include <libavformat/avio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


namespace dd::impl {
    inline void free_io_context(AVIOContext** ctx) noexcept {
        av_freep(&((*ctx)->buffer));
        avio_context_free(ctx);
    }
}


namespace dd {
    using format_context = impl::av_ptr<AVFormatContext, avformat_close_input>;
    using codec_context  = impl::av_ptr<AVCodecContext, avcodec_free_context>;
    using io_context     = impl::av_ptr<AVIOContext, impl::free_io_context>;
    using buffer_ref     = impl::av_ptr<AVBufferRef, av_buffer_unref>;
    using packet         = impl::av_ptr<AVPacket, av_packet_free>;
    using frame          = impl::av_ptr<AVFrame, av_frame_free>;

    template<typename T>
    using av_vector = std::vector<T, impl::av_allocator<T>>;
}