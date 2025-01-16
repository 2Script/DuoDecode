#pragma once
#include <cstddef>
#include <limits>
#include <utility>
extern "C" {
#include <libavutil/mem.h>
}

#if __cpp_exceptions
#include <new>
#define __DD_THROW(e) (throw (e))
#else
#include <cstdlib>
#define __DD_THROW(e) (std::abort())
#endif


namespace dd::impl {
    template<typename T>
    struct av_allocator {
        using pointer            = T*;
        using const_pointer      = T const*;
        using void_pointer       = void*;
        using const_void_pointer = void const*;
        using value_type         = T;
        using size_type          = std::size_t;
        using difference_type    = std::ptrdiff_t;

        template <typename U> struct rebind { using other = av_allocator<U>; };

        
        pointer allocate(size_type n, const_void_pointer = nullptr) {
            if(n == 0) return nullptr;
            av_max_alloc(max_size());
            pointer ret = static_cast<pointer>(av_malloc(n * sizeof(T)));
            if(!ret) __DD_THROW(std::bad_alloc());
            return ret;
        }
        
        void deallocate(pointer p, size_type) {
            av_free(p);
            p = nullptr;
        }

        size_type max_size() const noexcept { 
            return std::numeric_limits<std::size_t>::max() / sizeof(T); 
        }
        
        template<typename... Args>
        void construct(pointer p, Args&&... args) {
            new((void *)p) T(std::forward<Args>(args)...);
        }

        void destroy(pointer p) {
            p->~T();
        }
    };
}