#pragma once 
#include <cstddef>
#include <libavutil/buffer.h>
extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}


namespace dd::impl {
    //Replicates `FFHWDeviceContext` (https://www.ffmpeg.org/doxygen/trunk/structFFHWDeviceContext.html)
    struct device_context {
        AVHWDeviceContext p; 
        struct hardware_context const* hw_type;
        AVBufferRef* source_device;
    };
}

namespace dd::impl {
    //Replicates `HWContextType` (https://www.ffmpeg.org/doxygen/trunk/structHWContextType.html)
    struct hardware_context {
        AVHWDeviceType type;
        char const* name;
        AVPixelFormat const* pix_fmts;

        std::size_t device_hwctx_size;
        std::size_t device_hwconfig_size;
        std::size_t frames_hwctx_size;

        int  (*device_create)(AVHWDeviceContext* ctx, char const* device, AVDictionary* opts, int flags);
        int  (*device_derive)(AVHWDeviceContext* dst_ctx, AVHWDeviceContext* src_ctx, AVDictionary* opts, int flags);

        int  (*device_init)  (AVHWDeviceContext* ctx);
        void (*device_uninit)(AVHWDeviceContext* ctx);

        int  (*frames_get_constraints)(AVHWDeviceContext* ctx, void const* hwconfig, AVHWFramesConstraints* constraints);

        int  (*frames_init)  (AVHWFramesContext* ctx);
        void (*frames_uninit)(AVHWFramesContext* ctx);

        int  (*frames_get_buffer)   (AVHWFramesContext* ctx, AVFrame* frame);
        int  (*transfer_get_formats)(AVHWFramesContext* ctx, AVHWFrameTransferDirection dir, AVPixelFormat** formats);
        int  (*transfer_data_to)    (AVHWFramesContext* ctx, AVFrame* dst, AVFrame const* src);
        int  (*transfer_data_from)  (AVHWFramesContext* ctx, AVFrame* dst, AVFrame const* src);

        int  (*map_to)  (AVHWFramesContext* ctx, AVFrame* dst, AVFrame const* src, int flags);
        int  (*map_from)(AVHWFramesContext* ctx, AVFrame* dst, AVFrame const* src, int flags);

        int  (*frames_derive_to)  (AVHWFramesContext* dst_ctx, AVHWFramesContext* src_ctx, int flags);
        int  (*frames_derive_from)(AVHWFramesContext* dst_ctx, AVHWFramesContext* src_ctx, int flags);
    };
}