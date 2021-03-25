#pragma once

//// gcc/clang still don't support std::to_chars for float...
//#define sfbxUseCharconv

namespace sfbx {

#ifdef sfbxUseCharconv
template<class Value>
inline void append(std::string& dst, Value v)
{
    char buf[64];
    auto ret = std::to_chars(buf, buf + std::size(buf), v);
    if (ret.ec == std::errc{})
        dst.append(buf, ret.ptr);
}
#else

inline void append(std::string& dst, int16 v)
{
    char buf[64];
    sprintf(buf, "%hd", v);
    dst += buf;
}
inline void append(std::string& dst, int32 v)
{
    char buf[64];
    sprintf(buf, "%d", v);
    dst += buf;
}
inline void append(std::string& dst, int64 v)
{
    char buf[64];
    sprintf(buf, "%lld", v);
    dst += buf;
}
inline void append(std::string& dst, float32 v)
{
    char buf[64];
    sprintf(buf, "%g", v);
    dst += buf;
}
inline void append(std::string& dst, float64 v)
{
    char buf[64];
    sprintf(buf, "%lg", v);
    dst += buf;
}

#endif

inline void append(std::string& dst, boolean v)
{
    dst += v.value;
}


inline string_view& remove_leading_space(string_view& v)
{
    while (!v.empty() && std::isspace(v.front()))
        v.remove_prefix(1);
    return v;
}
inline string_view& remove_trailing_space(string_view& v)
{
    while (!v.empty() && std::isspace(v.back()))
        v.remove_suffix(1);
    return v;
}
inline string_view& remove_space(string_view& v)
{
    remove_leading_space(v);
    remove_trailing_space(v);
    return v;
}


template<class String, class Container, class ToString>
inline void join(String& dst, const Container& cont, typename String::const_pointer sep, const ToString& to_s)
{
    bool first = true;
    for (auto& v : cont) {
        if (!first)
            dst += sep;
        to_s(dst, v);
        first = false;
    }
}

template<class String, class Container>
inline void join(String& dst, const Container& cont, const char* sep)
{
    join(dst, cont, sep,
        [](String& dst, typename Container::const_reference v) { append(dst, v); });
}


template<class Body>
inline void split(string_view line, string_view sep, const Body& body)
{
    auto range = line;
    for (;;) {
        size_t pos = range.find(sep);
        if (pos != std::string::npos) {
            auto sub = range.substr(0, pos);
            body(remove_space(sub));
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


inline string_view get_line(string_view& str)
{
    size_t len = str.size();
    for (size_t i = 0; i < len; ++i) {
        if (str[i] == '\n' || str[i] == '\r') {
            auto ret = str.substr(0, i);
            str.remove_prefix(i + 1);
            if (!str.empty() && str.front() == '\n')
                str.remove_prefix(1);
            return ret;
        }
    }

    auto ret = str;
    str.remove_prefix(str.size());
    return ret;
}

inline std::string read_brace_block(std::istream& is)
{
    std::string ret;
    std::string line;
    auto add_line = [&]() {
        ret += line;
        ret += '\n';
    };

    int nest = 0;

    bool end = false;
    while (!is.eof()) {
        std::getline(is, line);

        if (line.find('{') != std::string::npos) {
            ++nest;
            add_line();
        }
        else if (line.find('}') != std::string::npos) {
            add_line();
            if (--nest == 0)
                break;
        }
        else {
            if (nest > 0)
                add_line();
        }
    }
    return ret;
}

inline string_view read_brace_block(string_view& is)
{
    string_view ret;
    size_t block_begin = 0;
    size_t block_end = 0;
    int nest = 0;

    bool end = false;
    size_t pos = 0;
    while (pos < is.size() && !end) {
        char c = is[pos];

        if (c == '{') {
            if (nest++ == 0) {
                block_begin = pos;
            }
        }
        else if (c == '}') {
            if (--nest == 0) {
                end = true;
                block_end = pos + 1;
            }
        }
        ++pos;
    }

    if (block_end >= block_begin) {
        auto ret = is.substr(block_begin, block_end - block_begin);
        is.remove_prefix(block_end);
        return ret;
    }
    else {
        return {};
    }
}

inline string_view read_until(string_view& is, string_view delim, bool include_delim)
{
    size_t src_size = is.size();
    size_t delim_size = delim.size();
    if (src_size < delim_size)
        return {};

    size_t count = src_size - delim_size;
    for (size_t i = 0; i < count; ++i) {
        if (is.substr(i, delim_size) == delim) {
            if (include_delim) {
                auto ret = is.substr(0, i + delim_size);
                is.remove_prefix(i + delim_size);
                return ret;
            }
            else {
                auto ret = is.substr(0, i);
                is.remove_prefix(i);
                return ret;
            }
        }
    }
    return {};
}

inline string_view read_n(string_view& is, size_t n)
{
    n = std::min(is.size(), n);
    auto ret = is.substr(0, n);
    is.remove_prefix(n);
    return ret;
}

inline void skip_n(string_view& is, size_t n)
{
    read_n(is, n);
}


inline bool is_empty_line(string_view line)
{
    remove_leading_space(line);
    return line.empty() || line.front() == ';'; // ';' : beginning of comment in ascii fbx
}

inline bool to_number(string_view str, float64& dst)
{
    remove_leading_space(str);
    if (!str.empty()) {
#ifdef sfbxUseCharconv
        return std::from_chars(str.data(), str.data() + str.size(), dst).ec == std::errc{};
#else
        if (std::isdigit(str.front()) || str.front() == '-') {
            dst = std::atof(str.data());
            return true;
        }
#endif
    }
    return false;
}

inline bool to_string(string_view str, string_view& dst)
{
    remove_leading_space(str);
    if (!str.empty() && str.front() == '"') {
        size_t len = str.size();
        for (size_t i = 1; i < len; ++i) {
            if (str[i] == '"') {
                dst = str.substr(1, i - 1);
                return true;
            }
        }
    }
    return false;
}

inline bool to_array_size(string_view str, size_t& dst)
{
    remove_leading_space(str);
    if (str.size() >= 2 && str[0] == '*') {
        skip_n(str, 1);
#ifdef sfbxUseCharconv
        return std::from_chars(str.data(), str.data() + str.size(), dst).ec == std::errc{};
#else
        dst = std::atoi(str.data());
        return true;
#endif
    }
    return false;
}
} // namespace sfbx
