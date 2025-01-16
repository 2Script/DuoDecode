#include "DuoDecode/impl/copy.hpp"
#include <zstring.hpp>
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/intreadwrite.h>
}


namespace dd::impl {
    void copy_audio(decoded_audio& dst, frame& src, std::size_t ch) noexcept {
        std::size_t frame_size = src->nb_samples * dst.bit_depth/8;
        auto old_size = dst.bytes[ch].size();
        dst.bytes[ch].resize(old_size + frame_size);
        zstring::memcpy(&dst.bytes[ch][old_size], src->extended_data[ch], frame_size);
        dst.channel_mask[ch] = true;
    };
}


namespace dd::impl {
    result<void> copy_video(decoded_video& dst, frame& src) noexcept {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(dst.format);
  
        if (!desc) return errc::invalid_argument;
  
        std::int32_t num_planes = 0;
        for (std::int32_t i = 0; i < desc->nb_components; ++i)
            num_planes = std::max(desc->comp[i].plane, num_planes);
        ++num_planes;
  
        std::array<int, 4> img_linesizes;
        std::int32_t ret = -av_image_fill_linesizes(img_linesizes.data(), dst.format, dst.width);
        if(ret > 0) return static_cast<errc>(ret);


        if((ret = -av_image_get_buffer_size(dst.format, dst.width, dst.height, 1)) > 0)
            return static_cast<errc>(ret);
        std::size_t frame_size = static_cast<std::size_t>(-ret);
        std::size_t old_size = dst.bytes.size();
        dst.bytes.resize(old_size + frame_size);

        std::byte* dst_ptr = &dst.bytes[old_size];
  

        for (std::int32_t i = 0; i < num_planes; i++) {
            std::uint8_t shift = ((i == 1 || i == 2) ? desc->log2_chroma_h : 0);
            std::uint8_t const* src_data = src->data[i];
            std::int32_t h = (static_cast<std::int32_t>(dst.height) + (1 << shift) - 1) >> shift;
  
            for (std::int32_t j = 0; j < h; ++j) {
                zstring::memcpy(dst_ptr, src_data, img_linesizes[i]);
                dst_ptr  += FFALIGN(img_linesizes[i], 1);
                src_data += src->linesize[i];
            }
        }
  
        if (!(desc->flags & AV_PIX_FMT_FLAG_PAL)) return {};

        std::uint32_t* d32 = reinterpret_cast<std::uint32_t*>(dst.bytes.data());
        for (std::uint16_t i = 0; i < 256; i++)
            AV_WL32(d32 + i, AV_RN32(src->data[1] + 4*i));

        return {};
    };
}