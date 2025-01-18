#include "DuoDecode/packet_handler.hpp"
#include <algorithm>
#include <zstring.hpp>
#include "DuoDecode/impl/byte_span.hpp"
#include "DuoDecode/impl/error.hpp"

namespace dd {
    namespace packet_handler{
        int read(void* src_data, std::uint8_t* dst, int dst_size) {
            impl::byte_span* src_span = static_cast<impl::byte_span*>(src_data);
            std::size_t remaining_size = std::min(static_cast<std::size_t>(dst_size), src_span->size);

            if(remaining_size == 0) return static_cast<int>(dd::errc::end_of_file);

            zstring::memcpy(dst, src_span->ptr, remaining_size);
            
            src_span->ptr += remaining_size;
            src_span->size -= remaining_size;
            return remaining_size;
        }
    }
}