#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxProperty.h"
#include "sfbxObject.h"
#include "sfbxParser.h"

#include <zlib.h>
#pragma comment(lib, "zlib.lib")


namespace sfbx {

uint32_t SizeOfElement(PropertyType type)
{
    switch (type) {
    case PropertyType::Int16:
    case PropertyType::Int16Array:
        return 2;

    case PropertyType::Int32:
    case PropertyType::Int32Array:
    case PropertyType::Float32:
    case PropertyType::Float32Array:
        return 4;

    case PropertyType::Int64:
    case PropertyType::Int64Array:
    case PropertyType::Float64:
    case PropertyType::Float64Array:
        return 8;

    case PropertyType::Bool:
    case PropertyType::BoolArray:
    case PropertyType::String:
    case PropertyType::Blob:
    default:
        return 1;
    }
}

Property::Property() {}

Property::Property(Property&& v) noexcept
    : m_type(v.m_type)
    , m_scalar(v.m_scalar)
    , m_data(std::move(v.m_data))
{}

void Property::read(std::istream& is)
{
    m_type = read1<PropertyType>(is);
    if (m_type == PropertyType::String || m_type == PropertyType::Blob) {
        uint32_t length = read1<uint32_t>(is);
        m_data.resize(length);
        is.read(m_data.data(), length);
    }
    else if (!isArray()) {
        switch (m_type) {
        case PropertyType::Bool:
            m_scalar.b = read1<boolean>(is);
            break;
        case PropertyType::Int16:
            m_scalar.i16 = read1<int16>(is);
            break;
        case PropertyType::Int32:
        case PropertyType::Float32:
            m_scalar.i32 = read1<int32>(is);
            break;
        case PropertyType::Int64:
        case PropertyType::Float64:
            m_scalar.i64 = read1<int64>(is);
            break;
        default:
            throw std::runtime_error(std::string("sfbx::Property::read(): Unsupported property type ") + std::to_string((char)m_type));
            break;
        }
    }
    else {
        uint32_t array_size = read1<uint32_t>(is);
        uint32_t encoding = read1<uint32_t>(is); // 0: plain 1: zlib-compressed

        uLong src_size = read1<uint32_t>(is);
        uLong dest_size = SizeOfElement(m_type) * array_size;
        m_data.resize(dest_size);

        if (encoding == 0) {
            readv(is, m_data.data(), dest_size);
        }
        else if (encoding == 1) {
            RawVector<char> compressed(src_size);
            readv(is, compressed.data(), src_size);
            uncompress((Bytef*)m_data.data(), &dest_size, (const Bytef*)compressed.data(), src_size);
        }
    }
}

void Property::write(std::ostream& os)
{
    writev(os, m_type);
    if (m_type == PropertyType::Blob || m_type == PropertyType::String) {
        writev(os, (uint32_t)m_data.size());
        writev(os, m_data);
    }
    else if (!isArray()) {
        // scalar
        switch (m_type) {
        case PropertyType::Bool:
            writev(os, m_scalar.b);
            break;
        case PropertyType::Int16:
            writev(os, m_scalar.i16);
            break;
        case PropertyType::Int32:
        case PropertyType::Float32:
            writev(os, m_scalar.i32);
            break;
        case PropertyType::Int64:
        case PropertyType::Float64:
            writev(os, m_scalar.i64);
            break;
        default:
            sfbxPrint("sfbx::Property::write(): Unsupported property type %c\n", (char)m_type);
            break;
        }
    }
    else {
        // array
        // use zlib if the data size is reasonably large.
        bool use_zlib = m_data.size() >= 128;

        if (use_zlib) {
            // with zlib compression
            writev(os, (uint32_t)getArraySize());
            writev(os, (uint32_t)1); // encoding: zlib

            uLong src_size = m_data.size();
            uLong dest_size = compressBound(src_size);
            RawVector<char> compressed(dest_size);
            compress((Bytef*)compressed.data(), &dest_size, (const Bytef*)m_data.data(), src_size);
            compressed.resize(dest_size);

            writev(os, (uint32_t)compressed.size());
            writev(os, compressed);
        }
        else {
            // without zlib compression
            writev(os, (uint32_t)getArraySize());
            writev(os, (uint32_t)0); // encoding: plain
            writev(os, (uint32_t)m_data.size());
            writev(os, m_data);
        }
    }
}


template<> span<int32> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Int32Array;
    m_data.resize(size * sizeof(int32));
    return make_span((int32*)m_data.data(), size);
}

template<> span<float32> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float32Array;
    m_data.resize(size * sizeof(float32));
    return make_span((float32*)m_data.data(), size);
}

template<> span<float64> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(float64));
    return make_span((float64*)m_data.data(), size);
}
template<> span<double2> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double2));
    return make_span((double2*)m_data.data(), size);
}
template<> span<double3> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double3));
    return make_span((double3*)m_data.data(), size);
}
template<> span<double4> Property::allocateArray(size_t size)
{
    m_type = PropertyType::Float64Array;
    m_data.resize(size * sizeof(double4));
    return make_span((double4*)m_data.data(), size);
}

template<class T>
static inline void Assign(RawVector<char>& dst, const span<T>& v)
{
    size_t s = v.size_bytes();
    dst.resize(s);
    dst.assign((char*)v.data(), s);
}

template<> void Property::assign(boolean v) { m_type = PropertyType::Bool; m_scalar.b = v; }
template<> void Property::assign(bool v)    { m_type = PropertyType::Bool; m_scalar.b = v; }
template<> void Property::assign(int16 v)   { m_type = PropertyType::Int16; m_scalar.i16 = v; }
template<> void Property::assign(int32 v)   { m_type = PropertyType::Int32; m_scalar.i32 = v; }
template<> void Property::assign(int64 v)   { m_type = PropertyType::Int64; m_scalar.i64 = v; }
template<> void Property::assign(float32 v) { m_type = PropertyType::Float32; m_scalar.f32 = v; }
template<> void Property::assign(float64 v) { m_type = PropertyType::Float64; m_scalar.f64 = v; }

template<> void Property::assign(double2 v)     { m_type = PropertyType::Float64Array; Assign(m_data, make_span(v)); }
template<> void Property::assign(double3 v)     { m_type = PropertyType::Float64Array; Assign(m_data, make_span(v)); }
template<> void Property::assign(double4 v)     { m_type = PropertyType::Float64Array; Assign(m_data, make_span(v)); }
template<> void Property::assign(double4x4 v)   { m_type = PropertyType::Float64Array; Assign(m_data, make_span(v)); }

template<> void Property::assign(span<uint8_t> v) { m_type = PropertyType::Blob; Assign(m_data, v); }
template<> void Property::assign(span<boolean> v) { m_type = PropertyType::BoolArray; Assign(m_data, v); }
template<> void Property::assign(span<int16> v)   { m_type = PropertyType::Int16Array; Assign(m_data, v); }
template<> void Property::assign(span<int32> v)   { m_type = PropertyType::Int32Array; Assign(m_data, v); }
template<> void Property::assign(span<int64> v)   { m_type = PropertyType::Int64Array; Assign(m_data, v); }
template<> void Property::assign(span<float32> v) { m_type = PropertyType::Float32Array; Assign(m_data, v); }
template<> void Property::assign(span<float64> v) { m_type = PropertyType::Float64Array; Assign(m_data, v); }

template<> void Property::assign(span<float2> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 2 }); }
template<> void Property::assign(span<float3> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 3 }); }
template<> void Property::assign(span<float4> v)  { assign(span<float32>{ (float32*)v.data(), v.size() * 4 }); }
template<> void Property::assign(span<double2> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 2 }); }
template<> void Property::assign(span<double3> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 3 }); }
template<> void Property::assign(span<double4> v) { assign(span<float64>{ (float64*)v.data(), v.size() * 4 }); }

void Property::assign(string_view v)
{
    m_type = PropertyType::String;
    m_data.assign(v.begin(), v.end());
}

PropertyType Property::getType() const
{
    return m_type;
}

bool Property::isArray() const
{
    return (char)m_type > 'Z';
}

uint64_t Property::getArraySize() const
{
    return m_data.size() / SizeOfElement(m_type);
}

template<> boolean Property::getValue() const { convert(PropertyType::Bool); return m_scalar.b; }
template<> bool    Property::getValue() const { convert(PropertyType::Bool); return m_scalar.b; }
template<> int16   Property::getValue() const { convert(PropertyType::Int16); return m_scalar.i16; }
template<> int32   Property::getValue() const { convert(PropertyType::Int32); return m_scalar.i32; }
template<> int64   Property::getValue() const { convert(PropertyType::Int64); return m_scalar.i64; }
template<> float32 Property::getValue() const { convert(PropertyType::Float32); return m_scalar.f32; }
template<> float64 Property::getValue() const { return m_scalar.f64; }

template<> double2 Property::getValue() const { return *(double2*)m_data.data(); }
template<> double3 Property::getValue() const { return *(double3*)m_data.data(); }
template<> double4 Property::getValue() const { return *(double4*)m_data.data(); }
template<> double4x4 Property::getValue() const { return *(double4x4*)m_data.data(); }

template<> string_view Property::getValue() const { return getString(); }

template<> span<int16>   Property::getArray() const { convert(PropertyType::Int16Array); return make_span((int16*)m_data.data(), getArraySize()); }
template<> span<int32>   Property::getArray() const { convert(PropertyType::Int32Array); return make_span((int32*)m_data.data(), getArraySize()); }
template<> span<int64>   Property::getArray() const { convert(PropertyType::Int64Array); return make_span((int64*)m_data.data(), getArraySize()); }
template<> span<float32> Property::getArray() const { convert(PropertyType::Float32Array); return make_span((float32*)m_data.data(), getArraySize()); }
template<> span<float64> Property::getArray() const { return make_span((float64*)m_data.data(), getArraySize()); }

template<> span<double2> Property::getArray() const { return make_span((double2*)m_data.data(), getArraySize() / 2); }
template<> span<double3> Property::getArray() const { return make_span((double3*)m_data.data(), getArraySize() / 3); }
template<> span<double4> Property::getArray() const { return make_span((double4*)m_data.data(), getArraySize() / 4); }

string_view Property::getString() const { return make_view(m_data); }

template<class T>
static inline void Convert(RawVector<char>& data)
{
    auto src = make_span((float64*)data.data(), data.size() / sizeof(float64));
    auto dst = make_span((T*)data.data(), src.size());
    copy(dst, src);
    data.resize(dst.size() * sizeof(T));
}

bool Property::convert(PropertyType t) const
{
    if (m_type == t)
        return true;

    bool ret = false;
    if (m_type == PropertyType::Float64) {
        switch (t) {
        case PropertyType::Bool: m_scalar.b = m_scalar.f64 == 0.0; ret = true; break;
        case PropertyType::Int16: m_scalar.i16 = (int16)m_scalar.f64; ret = true; break;
        case PropertyType::Int32: m_scalar.i32 = (int32)m_scalar.f64; ret = true; break;
        case PropertyType::Int64: m_scalar.i64 = (int64)m_scalar.f64; ret = true; break;
        case PropertyType::Float32: m_scalar.f32 = (float32)m_scalar.f64; ret = true; break;
        default: break;
        }
    }
    else if (m_type == PropertyType::Float64Array) {
        switch (t) {
        case PropertyType::Int16Array: Convert<int16>(m_data); ret = true; break;
        case PropertyType::Int32Array: Convert<int32>(m_data); ret = true; break;
        case PropertyType::Int64Array: Convert<int64>(m_data); ret = true; break;
        case PropertyType::Float32Array: Convert<float32>(m_data); ret = true; break;
        default: break;
        }
    }

    if (ret) {
        m_type = t;
        return true;
    }
    else {
        sfbxPrint("sfbx::Property::convert() invalid type conversion\n");
        return false;
    }
}

void Property::toString(std::string& dst, int depth) const
{
    if (isArray()) {
        auto do_append = [&dst, depth](const auto& span) {
            dst += "*";
            dst += std::to_string(span.size());
            dst += " {\n";
            AddTabs(dst, depth + 1);
            dst += "a: ";
            join(dst, span, ",");
            dst += "\n";
            AddTabs(dst, depth);
            dst += "}";
        };
        switch (m_type) {
        case PropertyType::Int16Array: do_append(getArray<int16>()); break;
        case PropertyType::Int32Array: do_append(getArray<int32>()); break;
        case PropertyType::Int64Array: do_append(getArray<int64>()); break;
        case PropertyType::Float32Array: do_append(getArray<float32>()); break;
        case PropertyType::Float64Array: do_append(getArray<float64>()); break;
        default: break;
        }
    }
    else {
        switch (m_type) {
        case PropertyType::Bool: append(dst, getValue<boolean>()); break;
        case PropertyType::Int16: append(dst, getValue<int16>()); break;
        case PropertyType::Int32: append(dst, getValue<int32>()); break;
        case PropertyType::Int64: append(dst, getValue<int64>()); break;
        case PropertyType::Float32: append(dst, getValue<float32>()); break;
        case PropertyType::Float64: append(dst, getValue<float64>()); break;

        case PropertyType::Blob:
        {
            dst += '"';
            dst += Base64Encode(make_span(m_data));
            dst += '"';
            break;
        }
        case PropertyType::String:
        {
            std::string s;
            s += " "; // just reserve space to avoid escape
            if (!m_data.empty()) {
                auto get_span = [](const char* s, size_t n) {
                    size_t i = 0;
                    for (; s[i] != '\0' && i < n; ++i) {}
                    return make_span(s, i);
                };

                string_view obj_name, class_name;
                if (SplitFullName(make_view(m_data), obj_name, class_name)) {
                    s.insert(s.end(), class_name.begin(), class_name.end());
                    s += "::";
                    s.insert(s.end(), obj_name.begin(), obj_name.end());
                }
                else {
                    s.insert(s.end(), m_data.begin(), m_data.end());
                }
                Escape(s);
            }
            s[0] = '"'; // replace reserved space
            s += '"';
            dst += s;
            break;
        }

        default:
            break;
        }
    }
}

} // namespace sfbx
