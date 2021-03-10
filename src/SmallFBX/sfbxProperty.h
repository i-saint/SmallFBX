#pragma once

#include "sfbxTypes.h"
#include "sfbxRawVector.h"
#include "sfbxAlgorithm.h"

namespace sfbx {

enum class PropertyType : uint8_t
{
    Unknown,

    Bool = 'C', // boolean (not built-in bool. see struct boolean in sfbxTypes.h)
    Int16 = 'Y', // int16
    Int32 = 'I', // int32
    Int64 = 'L', // int64
    Float32 = 'F', // float32
    Float64 = 'D', // float64

    String = 'S', // std::string
    Blob = 'R', // span<uint8_t>

    BoolArray = 'b', // span<boolean>
    Int16Array = 'y', // span<int16>
    Int32Array = 'i', // span<int32>
    Int64Array = 'l', // span<int64>
    Float32Array = 'f', // span<float32>
    Float64Array = 'd', // span<float64>
};
uint32_t SizeOfElement(PropertyType type);


template<class T> inline constexpr bool is_propery_pod = false;
#define PropPOD(T) template<> inline constexpr bool is_propery_pod<T> = true;
PropPOD(bool);
PropPOD(boolean);
PropPOD(int16);
PropPOD(int32);
PropPOD(int64);
PropPOD(float32);
PropPOD(float64);

PropPOD(double2);
PropPOD(double3);
PropPOD(double4);
PropPOD(double4x4);
#undef PropPOD


class Property
{
public:
    Property();
    Property(Property&& v);
    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;

    void read(std::istream& input);
    void write(std::ostream& output);

    template<class T> span<T> allocateArray(size_t size);

    template<class T, sfbxRestrict(is_propery_pod<T>)> void assign(T v);
    template<class T> void assign(span<T> v);
    template<class T> void assign(const RawVector<T>& v) { assign(make_span(v)); }
    template<class D, class S> void assign(array_adaptor<D, S> v) { copy(allocateArray<D>(v.values.size()), v.values); }
    void assign(string_view v);


    PropertyType getType() const;
    bool isArray() const;
    uint64_t getArraySize() const;

    template<class T> T getValue() const;
    template<class T> span<T> getArray() const;
    string_view getString() const;

    std::string toString(int depth = 0) const;

private:
    PropertyType m_type{};
    union {
        boolean b;
        int16 i16;
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
    } m_scalar{};
    RawVector<char> m_data;
};

} // namespace sfbx
