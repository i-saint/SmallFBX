#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxProperty.h"
#include "sfbxObject.h"

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

Property::Property(Property&& v)
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
            uncompress2((Bytef*)m_data.data(), &dest_size, (const Bytef*)compressed.data(), &src_size);
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

template<> boolean Property::getValue() const { return m_scalar.b; }
template<> bool    Property::getValue() const { return m_scalar.b; }
template<> int16   Property::getValue() const { return m_scalar.i16; }
template<> int32   Property::getValue() const { return m_scalar.i32; }
template<> int64   Property::getValue() const { return m_scalar.i64; }
template<> float32 Property::getValue() const { return m_scalar.f32; }
template<> float64 Property::getValue() const { return m_scalar.f64; }

template<> double2 Property::getValue() const { return *(double2*)m_data.data(); }
template<> double3 Property::getValue() const { return *(double3*)m_data.data(); }
template<> double4 Property::getValue() const { return *(double4*)m_data.data(); }
template<> double4x4 Property::getValue() const { return *(double4x4*)m_data.data(); }

template<> span<boolean> Property::getArray() const { return make_span((boolean*)m_data.data(), getArraySize()); }
template<> span<int16>   Property::getArray() const { return make_span((int16*)m_data.data(), getArraySize()); }
template<> span<int32>   Property::getArray() const { return make_span((int32*)m_data.data(), getArraySize()); }
template<> span<int64>   Property::getArray() const { return make_span((int64*)m_data.data(), getArraySize()); }
template<> span<float32> Property::getArray() const { return make_span((float32*)m_data.data(), getArraySize()); }
template<> span<float64> Property::getArray() const { return make_span((float64*)m_data.data(), getArraySize()); }

template<> span<double2> Property::getArray() const { return make_span((double2*)m_data.data(), getArraySize() / 2); }
template<> span<double3> Property::getArray() const { return make_span((double3*)m_data.data(), getArraySize() / 3); }
template<> span<double4> Property::getArray() const { return make_span((double4*)m_data.data(), getArraySize() / 4); }

string_view Property::getString() const { return make_view(m_data); }

std::string Property::toString(int depth) const
{
    if (isArray()) {
        auto toS = [depth](const auto& span) {
            std::string s = "*";
            s += std::to_string(span.size());
            s += " {\n";
            AddTabs(s, depth + 1);
            s += "a: ";
            join(s, span, ",");
            s += "\n";
            AddTabs(s, depth);
            s += "}";
            return s;
        };
        switch (m_type) {
        case PropertyType::BoolArray: return toS(getArray<boolean>());
        case PropertyType::Int16Array: return toS(getArray<int16>());
        case PropertyType::Int32Array: return toS(getArray<int32>());
        case PropertyType::Int64Array: return toS(getArray<int64>());
        case PropertyType::Float32Array: return toS(getArray<float32>());
        case PropertyType::Float64Array: return toS(getArray<float64>());
        default: break;
        }
    }
    else {
        switch (m_type) {
        case PropertyType::Bool: return std::to_string(getValue<boolean>());
        case PropertyType::Int16: return std::to_string(getValue<int16>());
        case PropertyType::Int32: return std::to_string(getValue<int32>());
        case PropertyType::Int64: return std::to_string(getValue<int64>());
        case PropertyType::Float32: return std::to_string(getValue<float32>());
        case PropertyType::Float64: return std::to_string(getValue<float64>());

        case PropertyType::Blob:
        {
            std::string s;
            s += '"';
            s += Base64Encode(make_span(m_data));
            s += '"';
            return s;
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
            return s;
        }

        default:
            break;
        }
    }
    return "";
}

} // namespace sfbx
