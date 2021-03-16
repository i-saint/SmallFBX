#pragma once
#include <cstring>
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

template<class T, sfbxRestrict(std::is_pointer_v<T>)> inline T get_raw_ptr(T v) { return v; }
template<class T> inline T* get_raw_ptr(const std::shared_ptr<T>& v) { return v.get(); }


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


inline string_view remove_leading_space(string_view v)
{
    while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.remove_prefix(1);
    return v;
}
inline string_view remove_trailing_space(string_view v)
{
    while (!v.empty() && (v.back() == ' ' || v.back() == '\t')) v.remove_suffix(1);
    return v;
}
inline string_view remove_space(string_view v)
{
    v = remove_leading_space(v);
    v = remove_trailing_space(v);
    return v;
}

template<class Body>
inline void split(string_view line, string_view sep, const Body& body)
{
    auto range = line;
    for (;;) {
        size_t pos = range.find(sep);
        if (pos != std::string::npos) {
            body(remove_space(range.substr(0, pos)));
            range.remove_prefix(pos + sep.size());
        }
        else {
            body(remove_space(range));
            break;
        }
    }
}

inline std::vector<string_view> split(string_view line, string_view sep)
{
    std::vector<string_view> ret;
    split(line, sep, [&ret](string_view e) { ret.push_back(e); });
    return ret;
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
inline get_value_type<Container> find(const Container& cont, const get_value_type<Container>& v)
{
    for (auto& c : cont)
        if (c == v)
            return c;
    return {};
}

template<class Container, class Condition>
inline get_value_type<Container> find_if(const Container& cont, const Condition& cond)
{
    for (auto& c : cont)
        if (cond(c))
            return c;
    return {};
}

template<class Container, class T>
inline bool erase(Container& cont, const T& v)
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

template<class Container, class T>
inline bool replace(Container& cont, const T& before, const T& after)
{
    for (auto& c : cont) {
        if (c == before) {
            c = after;
            return true;
        }
    }
    return false;
}


// substitution for std::string_view::starts_with() (which require C++20)
inline bool starts_with(string_view str, string_view v)
{
    return std::strncmp(str.data(), v.data(), v.size()) == 0;
}

} // namespace sfbx
