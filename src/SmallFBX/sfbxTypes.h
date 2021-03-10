#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <type_traits>
#ifdef __cpp_lib_span
    #include <span>
#endif
#include "sfbxMeta.h"

namespace sfbx {


// SmallFBX require C++17, but not C++20. use std::span only if C++20 is available and our own version if not.

#ifdef __cpp_lib_span

template<class T> using span = std::span<T>;

#else

template<class T>
class span
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    span() {}
    span(const T* d, size_t s) : m_data(const_cast<T*>(d)), m_size(s) {}

    template<size_t N>
    span(const T(&v)[N]) : m_data(const_cast<T*>(v)), m_size(N) {}

    template<class Cont, sfbxRestrict(is_contiguous_container<Cont>)>
    span(const Cont& v) : m_data(const_cast<T*>(v.data())), m_size(v.size()) {}

    span& operator=(const span& v) { m_data = const_cast<T*>(v.m_data); m_size = v.m_size; return *this; }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    size_t size_bytes() const { return sizeof(T) * m_size; }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T& operator[](size_t i) { return m_data[i]; }
    const T& operator[](size_t i) const { return m_data[i]; }

    T& front() { return m_data[0]; }
    const T& front() const { return m_data[0]; }
    T& back() { return m_data[m_size - 1]; }
    const T& back() const { return m_data[m_size - 1]; }

    iterator begin() { return m_data; }
    const_iterator begin() const { return m_data; }
    iterator end() { return m_data + m_size; }
    const_iterator end() const { return m_data + m_size; }

private:
    T* m_data{};
    size_t m_size{};
};
#endif

template<class T, size_t N>
struct array_to_span
{
    span<T> operator()(const T(&v)[N]) const { return { const_cast<T*>(v), N }; }
};
template<size_t N>
struct array_to_span<char, N>
{
    span<char> operator()(const char(&v)[N]) const { return { const_cast<char*>(v), N - 1 }; } // -1 to ignore null terminator
};
// from array
template<class T, size_t N>
inline constexpr span<T> make_span(const T (&v)[N]) { return array_to_span<T, N>()(v); }

// from contagious container (std::vector, etc)
template<class Cont, sfbxRestrict(is_contiguous_container<Cont>)>
inline constexpr span<get_value_type<Cont>> make_span(const Cont& v) { return { const_cast<get_value_type<Cont>*>(v.data()), v.size() }; }

// from pointer & size
template<class T>
inline constexpr span<T> make_span(const T* v, size_t n) { return { const_cast<T*>(v), n }; }


using std::string_view;

template<size_t N>
inline constexpr string_view make_view(const char(&v)[N]) { return string_view{ v, N - 1 }; } // -1 to ignore null terminator

template<class Cont, sfbxRestrict(is_contiguous_container<Cont>)>
inline constexpr string_view make_view(const Cont& v) { return { const_cast<get_value_type<Cont>*>(v.data()), v.size() }; }


using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;

template<class T>
struct tvec2
{
    using value_type = T;

    T x, y;

    static constexpr size_t size() { return 2; }
    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec2& v) const { return x == v.x && y == v.y; }
    bool operator!=(const tvec2& v) const { return !((*this) == v); }
    template<class U> operator tvec2<U>() const { return { (U)x, (U)y }; }

    template<class U> void assign(const U* v)
    {
        T* d = data();
        for (size_t i = 0; i < size(); ++i)
            *d++ = T(*v++);
    }
    template<class U> void operator=(span<U> v) { assign(v.data()); }

    static constexpr tvec2 zero() { return { (T)0, (T)0 }; }
    static constexpr tvec2 one()  { return { (T)1, (T)1 }; }
};

template<class T>
struct tvec3
{
    using value_type = T;

    T x, y, z;

    static constexpr size_t size() { return 3; }
    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec3& v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const tvec3& v) const { return !((*this) == v); }
    template<class U> operator tvec3<U>() const { return { (U)x, (U)y, (U)z }; }

    template<class U> void assign(const U* v)
    {
        T* d = data();
        for (size_t i = 0; i < size(); ++i)
            *d++ = T(*v++);
    }
    template<class U> void operator=(span<U> v) { assign(v.data()); }

    static constexpr tvec3 zero() { return { (T)0, (T)0, (T)0 }; }
    static constexpr tvec3 one()  { return { (T)1, (T)1, (T)1 }; }
};

template<class T>
struct tvec4
{
    using value_type = T;

    T x, y, z, w;

    static constexpr size_t size() { return 4; }
    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tvec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const tvec4& v) const { return !((*this) == v); }
    template<class U> operator tvec4<U>() const { return { (U)x, (U)y, (U)z, (U)w }; }

    template<class U> void assign(const U* v)
    {
        T* d = data();
        for (size_t i = 0; i < size(); ++i)
            *d++ = T(*v++);
    }
    template<class U> void operator=(span<U> v) { assign(v.data()); }

    static constexpr tvec4 zero() { return { (T)0, (T)0, (T)0, (T)0 }; }
    static constexpr tvec4 one()  { return { (T)1, (T)1, (T)1, (T)1 }; }
};

template<class T>
struct tquat
{
    using value_type = T;

    T x, y, z, w;

    static constexpr size_t size() { return 4; }
    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    T& operator[](int i) { return data()[i]; }
    const T& operator[](int i) const { return data()[i]; }
    bool operator==(const tquat& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const tquat& v) const { return !((*this) == v); }
    template<class U> operator tquat<U>() const { return { (U)x, (U)y, (U)z, (U)w }; }

    template<class U> void assign(const U* v)
    {
        T* d = data();
        for (size_t i = 0; i < size(); ++i)
            *d++ = T(*v++);
    }
    template<class U> void operator=(span<U> v) { assign(v.data()); }

    static constexpr tquat identity() { return{ (T)0, (T)0, (T)0, (T)1 }; }
};

template<class T>
struct tmat4x4
{
    using value_type = T;

    tvec4<T> m[4];

    static constexpr size_t size() { return 16; }
    T* data() { return (T*)this; }
    const T* data() const { return (T*)this; }
    tvec4<T>& operator[](int i) { return m[i]; }
    const tvec4<T>& operator[](int i) const { return m[i]; }

    bool operator==(const tmat4x4& v) const
    {
        for (size_t i = 0; i < size(); ++i)
            if (data()[i] != v.data()[i])
                return false;
        return true;
    }
    bool operator!=(const tmat4x4& v) const { return !((*this) == v); }

    template<class U> operator tmat4x4<U>() const
    {
        return { { tvec4<U>(m[0]), tvec4<U>(m[1]), tvec4<U>(m[2]), tvec4<U>(m[3]) } };
    }

    template<class U> void assign(const U* v)
    {
        T* d = data();
        for (size_t i = 0; i < size(); ++i)
            *d++ = T(*v++);
    }
    template<class U> void operator=(span<U> v) { assign(v.data()); }

    static constexpr tmat4x4 identity()
    {
        return{ {
            { (T)1, (T)0, (T)0, (T)0 },
            { (T)0, (T)1, (T)0, (T)0 },
            { (T)0, (T)0, (T)1, (T)0 },
            { (T)0, (T)0, (T)0, (T)1 }
        } };
    }
};

template<class T> inline constexpr size_t get_vector_size = 1;
template<class T> inline constexpr size_t get_vector_size<tvec2<T>> = 2;
template<class T> inline constexpr size_t get_vector_size<tvec3<T>> = 3;
template<class T> inline constexpr size_t get_vector_size<tvec4<T>> = 4;
template<class T> inline constexpr size_t get_vector_size<tquat<T>> = 4;
template<class T> inline constexpr size_t get_vector_size<tmat4x4<T>> = 16;

template<class T> inline constexpr bool is_scalar = get_vector_size<T> == 1;
template<class T> inline constexpr bool is_vector = get_vector_size<T> != 1;

template<class T> struct get_scalar { using type = T; };
template<class T> struct get_scalar<tvec2<T>> { using type = T; };
template<class T> struct get_scalar<tvec3<T>> { using type = T; };
template<class T> struct get_scalar<tvec4<T>> { using type = T; };
template<class T> struct get_scalar<tquat<T>> { using type = T; };
template<class T> struct get_scalar<tmat4x4<T>> { using type = T; };
template<class T> using get_scalar_t = typename get_scalar<T>::type;

using int2 = tvec2<int>;
using int3 = tvec3<int>;
using int4 = tvec4<int>;

using float2 = tvec2<float>;
using float3 = tvec3<float>;
using float4 = tvec4<float>;
using quatf = tquat<float>;
using float4x4 = tmat4x4<float>;

using double2 = tvec2<double>;
using double3 = tvec3<double>;
using double4 = tvec4<double>;
using quatd = tquat<double>;
using double4x4 = tmat4x4<double>;


enum class RotationOrder : int
{
    XYZ,
    XZY,
    YZX,
    YXZ,
    ZXY,
    ZYX,
    SphericXYZ
};


// FBX's bool property represents true as 'Y' and false as 'T'
struct boolean
{
    char value;

    operator bool() const { return value == 'Y'; }
    void operator=(bool v) { value = v ? 'Y' : 'T'; }
};


template<class D, class S>
struct array_adaptor
{
    using dst_type = D;
    using src_type = S;

    span<S> values;
    template<class... T> array_adaptor(T&&... v) : values(make_span(v...)) {}
};
template<class D, class S, class... T>
inline auto make_adaptor(const S& src, T&&... a) { return array_adaptor<D, get_value_type<S>>(src, a...); }


class Node; using NodePtr = std::shared_ptr<Node>;
class Object; using ObjectPtr = std::shared_ptr<Object>;
class Document; using DocumentPtr = std::shared_ptr<Document>;

template<class T, class U>
inline T* as(U* v) { return dynamic_cast<T*>(v); }

} // namespace sfbx


namespace std {

inline std::string to_string(sfbx::boolean v)
{
    std::string ret;
    ret += v.value;
    return ret;
}

} // namespace std
