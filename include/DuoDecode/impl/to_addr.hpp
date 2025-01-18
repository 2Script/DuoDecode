#pragma once
#include <memory>
#include <type_traits>

namespace dd::impl {
    template<typename, typename = void>
    struct has_ptr_traits_to_addr : std::false_type {};
    
    template<typename T>
    struct has_ptr_traits_to_addr<T, std::void_t<decltype(std::pointer_traits<T>::to_address(std::declval<T>()))>> : std::true_type {};
}


namespace dd::impl {
    template<typename T>
    constexpr T* to_addr(T* p) noexcept {
        static_assert(!std::is_function_v<T>);
        return p;
    }
    
    template<class T>
    constexpr auto to_addr(const T& p) noexcept {
        if constexpr (has_ptr_traits_to_addr<T>::value)
            return std::pointer_traits<T>::to_address(p);
        else
            return to_addr(p.operator->());
    }
}