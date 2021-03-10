#pragma once
#include "sfbxTypes.h"
#include "sfbxAlgorithm.h"
#include "sfbxMath.h"
#include "sfbxTokens.h"
#include "sfbxNode.h"
#include "sfbxUtil.h"

#define sfbxEnableLegacyFormatSupport

namespace sfbx {

template<class T, sfbxRestrict(std::is_pod_v<T> && !std::is_pointer_v<T>)>
inline T read1(std::istream& is)
{
    T r;
    is.read((char*)&r, sizeof(T));
    return r;
}
inline void readv(std::istream& is, void* dst, size_t size)
{
    is.read((char*)dst, size);
}
inline void readv(std::istream& is, std::string& dst, size_t s)
{
    dst.resize(s);
    is.read(dst.data(), s);
}

template<class T, sfbxRestrict(std::is_pod_v<T> && !std::is_pointer_v<T>)>
inline void writev(std::ostream& os, T v)
{
    os.write((const char*)&v, sizeof(T));
}
inline void writev(std::ostream& os, const void* src, size_t size)
{
    os.write((const char*)src, size);
}
template<class T>
inline void writev(std::ostream& os, span<T> v)
{
    writev(os, v.data(), v.size_bytes());
}
template<class Cont, sfbxRestrict(is_contiguous_container<Cont>)>
inline void writev(std::ostream& os, const Cont& v)
{
    writev(os, make_span(v));
}
template<class T, size_t N>
inline void writev(std::ostream& os, const T(&v)[N])
{
    writev(os, make_span(v));
}


inline void AddTabs(std::string& dst, int n)
{
    for (int i = 0; i < n; ++i)
        dst += '\t';
}

inline int64 ToTicks(float v)
{
    return int64((float64)v * sfbxI_TicksPerSecond);
}
inline float FromTicks(int64 v)
{
    return float((float64)v / sfbxI_TicksPerSecond);
}


inline size_t GetPropertyCount(Node* node)
{
    if (node)
        return node->getProperties().size();
    return 0;
}

inline PropertyType GetPropertyType(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getType();
    return PropertyType::Unknown;
}

template<class T, sfbxRestrict(is_scalar<T>)>
inline T GetPropertyValue(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getValue<T>();
    return {};
}

inline string_view GetPropertyString(Node* node, size_t pi = 0)
{
    if (node)
        if (Property* prop = node->getProperty(pi))
            return prop->getString();
    return {};
}


template<class T, class Dst, sfbxRestrict(is_RawVector<Dst>)>
inline void GetPropertyValue(Dst& dst, Node* node)
{
    if (node) {
        if (Property* prop = node->getProperty(0)) {
            if (prop->isArray())
                dst = prop->getArray<T>();
#ifdef sfbxEnableLegacyFormatSupport
            else
                node->getPropertiesValues<T>(dst);
#endif
        }
    }
}
template<class T, class Dst, sfbxRestrict(is_vector<Dst>)>
inline void GetPropertyValue(Dst& dst, Node* node)
{
    if (node) {
        if (Property* prop = node->getProperty(0)) {
            if (prop->isArray())
                dst = prop->getValue<T>();
#ifdef sfbxEnableLegacyFormatSupport
            else
                node->getPropertiesValues<T>(dst);
#endif
        }
    }
}

template<class T, sfbxRestrict(is_scalar<T>)>
inline T GetChildPropertyValue(Node* node, string_view name, size_t pi = 0)
{
    if (node)
        return GetPropertyValue<T>(node->findChild(name), pi);
    return {};
}
template<class T, class Dst, sfbxRestrict(is_RawVector<Dst> || is_vector<Dst>)>
inline void GetChildPropertyValue(Dst& dst, Node* node, string_view name)
{
    if (node)
        GetPropertyValue<T>(dst, node->findChild(name));
}
inline string_view GetChildPropertyString(Node* node, string_view name, size_t pi = 0)
{
    if (node)
        return GetPropertyString(node->findChild(name));
    return {};
}


template<class Body>
inline void EnumerateProperties(Node* n, const Body& body)
{
    for (Node* props : n->getChildren()) {
        if (starts_with(props->getName(), sfbxS_Properties)) {
            for (Node* p : props->getChildren())
                body(p);
            break;
        }
    }
}

class CounterStream : public std::ostream
{
public:
    CounterStream();
    uint64_t size();
    void reset();

private:
    class StreamBuf : public std::streambuf
    {
    public:
        static char s_dummy_buf[1024];

        StreamBuf();
        int overflow(int c) override;
        int sync() override;
        void reset();

        uint64_t m_size = 0;
    } m_buf;
};

} // namespace sfbx
