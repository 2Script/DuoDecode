#pragma once
#include <vector>
#include "DuoDecode/impl/av_ptr.hpp"
#include "DuoDecode/impl/av_allocator.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


namespace dd {
    using format_context = impl::av_ptr<AVFormatContext, avformat_close_input>;
    using codec_context  = impl::av_ptr<AVCodecContext, avcodec_free_context>;
    using buffer_ref     = impl::av_ptr<AVBufferRef, av_buffer_unref>;
    using packet         = impl::av_ptr<AVPacket, av_packet_free>;
    using frame          = impl::av_ptr<AVFrame, av_frame_free>;

    template<typename T>
    using av_vector = std::vector<T, impl::av_allocator<T>>;
}

namespace dd {
    struct io_context : public impl::av_ptr<AVIOContext, avio_context_free> {
        using impl::av_ptr<AVIOContext, avio_context_free>::av_ptr;
        using impl::av_ptr<AVIOContext, avio_context_free>::operator=;
        io_context(io_context&&) noexcept = default;
        io_context& operator=(io_context&&) noexcept = default;
        //copy constructor/assignment is implicitly deleted because of av_ptr

        ~io_context() noexcept { if(handle) av_freep(&handle->buffer); }
    };
}