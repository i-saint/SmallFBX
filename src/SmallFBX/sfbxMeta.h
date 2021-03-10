#pragma once
#include <type_traits>

#define sfbxRestrict(...) std::enable_if_t<__VA_ARGS__, bool> = true

namespace sfbx {

// is_contiguous_container<T> : true if T has data() and size(). intended to detect std::vector, sfbx::RawVector and sfbx::span.
template<class T, class = void>
inline constexpr bool is_contiguous_container = false;
template<class T>
inline constexpr bool is_contiguous_container<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>> = true;


template<class T, class = void>
inline constexpr bool has_value_type = false;
template<class T>
inline constexpr bool has_value_type<T, std::void_t<typename T::value_type>> = true;

template<class T, sfbxRestrict(has_value_type<T>)> inline typename T::value_type get_value_type_impl(const T&);
template<class T, sfbxRestrict(std::is_array_v<T>)> inline T get_value_type_impl(const T&);
template<class T, sfbxRestrict(std::is_pointer_v<T>)> inline std::remove_pointer_t<T> get_value_type_impl(T);
template<class T, sfbxRestrict(std::is_scalar_v<T> && !std::is_pointer_v<T>)> inline T get_value_type_impl(T);
template<class T> using get_value_type = decltype(get_value_type_impl(std::declval<T>()));


template<class T, class = void>
inline constexpr bool has_resize = false;
template<class T>
inline constexpr bool has_resize<T, std::void_t<decltype(std::declval<T>().resize(0))>> = true;

} // namespace sfbx
