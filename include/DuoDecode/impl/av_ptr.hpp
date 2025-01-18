#pragma once 
extern "C" {
#include <libavutil/mem.h>
}

namespace dd::impl {
    template<typename T, void(&DeleterFn)(T**)>
    class av_ptr {
    public:
        constexpr av_ptr() noexcept = default;
        ~av_ptr() noexcept { reset(); }

        constexpr T** operator&() noexcept { return &handle; }
        constexpr T* const* operator&() const noexcept { return &handle; }
        constexpr operator T*() const noexcept { return handle; }
        constexpr explicit operator bool() const noexcept { return static_cast<bool>(handle); }
        
        constexpr T& operator*() const noexcept { return *handle; };
        constexpr T* operator->() const noexcept { return handle; };
    

    public:
        constexpr av_ptr(av_ptr&& other) noexcept : 
            handle(other.handle) { 
            other.handle = nullptr; 
        }
        constexpr av_ptr& operator=(av_ptr&& other) noexcept {
            reset(other.handle);
            other.handle = nullptr;
            return *this;
        };

        av_ptr(const av_ptr& other) = delete;
        av_ptr& operator=(const av_ptr& other) = delete;


    public:
        constexpr av_ptr& operator=(T* ptr) noexcept {
            reset(ptr);
            return *this;
        };
    
        constexpr void reset(T* ptr = nullptr) noexcept {
            if(handle && handle != ptr) DeleterFn(&handle);
            handle = ptr;
        }
    

    protected:
        T* handle = nullptr;
    };
}