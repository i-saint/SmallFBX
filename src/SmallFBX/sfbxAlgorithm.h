#pragma once

#include "sfbxTypes.h"

namespace sfbx {

// call resize() if Cont has. otherwise do nothing.
template <class Cont>
inline void resize(Cont& dst, size_t n)
{
    if constexpr (has_resize<Cont>) {
        dst.resize(n);
    }
}


template<class Container, class Body>
inline void each(Container& val, const Body& body)
{
    for (auto& v : val)
        body(v);
}

template<class Container, class Indices, class Body>
inline void each_indexed(Container& val, Indices& idx, const Body& body)
{
    for (auto i : idx)
        body(val[i]);
}


template<class D, class S>
inline void copy(D* dst, const S* src, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        *dst++ = *src++;
}
template<class D, class S>
inline void copy(span<D> dst, span<S> src)
{
    copy(dst.data(), src.data(), src.size());
}
template<class Dst, class Src>
inline void copy(Dst& dst, const Src& src)
{
    resize(dst, src.size());
    copy(make_span(dst), make_span(src));
}

template<class Dst, class Src, class Indices>
inline void copy_indexed(Dst& dst, const Src& src, const Indices& idx)
{
    resize(dst, idx.size());
    auto* d = dst.data();
    for (auto i : idx)
        *d++ = src[i];
}


template<class Dst, class Src, class Body>
inline void transform(Dst& dst, const Src& src, const Body& body)
{
    resize(dst, src.size());
    auto* d = dst.data();
    for (const auto& v : src)
        *d++ = body(v);
}

template<class Dst, class Src, class Indices, class Body>
inline void transform_indexed(Dst& dst, const Src& src, const Indices& idx, const Body& body)
{
    resize(dst, idx.size());
    auto* d = dst.data();
    for (auto i : idx)
        *d++ = body(src[i]);
}


template<class String, class Container, class ToString>
inline void join(String& dst, const Container& cont, typename String::const_pointer sep, const ToString& to_s)
{
    bool first = true;
    for (auto& v : cont) {
        if (!first)
            dst += sep;
        dst += to_s(v);
        first = false;
    }
}

template<class Container>
inline void join(std::string& dst, const Container& cont, const char* sep)
{
    join(dst, cont, sep,
        [](typename Container::const_reference v) { return std::to_string(v); });
}


template<class Container, class Body>
inline size_t count(const Container& cont, const Body& body)
{
    size_t r = 0;
    for (auto& v : cont)
        if (body(v))
            ++r;
    return r;
}

template<class Container>
inline bool erase(Container& cont, get_value_type<Container> v)
{
    auto it = std::find(cont.begin(), cont.end(), v);
    if (it != cont.end()) {
        cont.erase(it);
        return true;
    }
    return false;
}

template<class Container, class Body>
inline bool erase_if(Container& cont, const Body& v)
{
    auto it = std::remove_if(cont.begin(), cont.end(), v);
    if (it != cont.end()) {
        cont.erase(it, cont.end());
        return true;
    }
    return false;
}


// substitution for std::string_view::starts_with() (which require C++20)
inline bool starts_with(string_view str, string_view v)
{
    return std::strncmp(str.data(), v.data(), v.size()) == 0;
}

} // namespace sfbx
