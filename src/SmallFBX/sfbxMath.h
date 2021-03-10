#pragma once
#include <cmath>
#include "sfbxTypes.h"

namespace sfbx {

constexpr float PI = 3.14159265358979323846264338327950288419716939937510f;
constexpr float DegToRad = PI / 180.0f;

template<class T> inline tvec2<T> operator-(const tvec2<T>& v) { return{ -v.x, -v.y }; }
template<class T, class U> inline tvec2<T> operator+(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x + r.x, l.y + r.y }; }
template<class T, class U> inline tvec2<T> operator-(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x - r.x, l.y - r.y }; }
template<class T, class U> inline tvec2<T> operator*(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x * r.x, l.y * r.y }; }
template<class T, class U> inline tvec2<T> operator/(const tvec2<T>& l, const tvec2<U>& r) { return{ l.x / r.x, l.y / r.y }; }
template<class T> inline tvec2<T> operator+(T l, const tvec2<T>& r) { return{ l + r.x, l + r.y }; }
template<class T> inline tvec2<T> operator-(T l, const tvec2<T>& r) { return{ l - r.x, l - r.y }; }
template<class T> inline tvec2<T> operator*(T l, const tvec2<T>& r) { return{ l * r.x, l * r.y }; }
template<class T> inline tvec2<T> operator/(T l, const tvec2<T>& r) { return{ l / r.x, l / r.y }; }
template<class T> inline tvec2<T> operator+(const tvec2<T>& l, T r) { return{ l.x + r, l.y + r }; }
template<class T> inline tvec2<T> operator-(const tvec2<T>& l, T r) { return{ l.x - r, l.y - r }; }
template<class T> inline tvec2<T> operator*(const tvec2<T>& l, T r) { return{ l.x * r, l.y * r }; }
template<class T> inline tvec2<T> operator/(const tvec2<T>& l, T r) { return{ l.x / r, l.y / r }; }
template<class T, class U> inline tvec2<T>& operator+=(tvec2<T>& l, const tvec2<U>& r) { l.x += r.x; l.y += r.y; return l; }
template<class T, class U> inline tvec2<T>& operator-=(tvec2<T>& l, const tvec2<U>& r) { l.x -= r.x; l.y -= r.y; return l; }
template<class T, class U> inline tvec2<T>& operator*=(tvec2<T>& l, const tvec2<U>& r) { l.x *= r.x; l.y *= r.y; return l; }
template<class T, class U> inline tvec2<T>& operator/=(tvec2<T>& l, const tvec2<U>& r) { l.x /= r.x; l.y /= r.y; return l; }
template<class T> inline tvec2<T>& operator+=(tvec2<T>& l, T r) { l.x += r; l.y += r; return l; }
template<class T> inline tvec2<T>& operator-=(tvec2<T>& l, T r) { l.x -= r; l.y -= r; return l; }
template<class T> inline tvec2<T>& operator*=(tvec2<T>& l, T r) { l.x *= r; l.y *= r; return l; }
template<class T> inline tvec2<T>& operator/=(tvec2<T>& l, T r) { l.x /= r; l.y /= r; return l; }

template<class T> inline tvec3<T> operator-(const tvec3<T>& v) { return{ -v.x, -v.y, -v.z }; }
template<class T, class U> inline tvec3<T> operator+(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x + r.x, l.y + r.y, l.z + r.z }; }
template<class T, class U> inline tvec3<T> operator-(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x - r.x, l.y - r.y, l.z - r.z }; }
template<class T, class U> inline tvec3<T> operator*(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x * r.x, l.y * r.y, l.z * r.z }; }
template<class T, class U> inline tvec3<T> operator/(const tvec3<T>& l, const tvec3<U>& r) { return{ l.x / r.x, l.y / r.y, l.z / r.z }; }
template<class T> inline tvec3<T> operator+(T l, const tvec3<T>& r) { return{ l + r.x, l + r.y, l + r.z }; }
template<class T> inline tvec3<T> operator-(T l, const tvec3<T>& r) { return{ l - r.x, l - r.y, l - r.z }; }
template<class T> inline tvec3<T> operator*(T l, const tvec3<T>& r) { return{ l * r.x, l * r.y, l * r.z }; }
template<class T> inline tvec3<T> operator/(T l, const tvec3<T>& r) { return{ l / r.x, l / r.y, l / r.z }; }
template<class T> inline tvec3<T> operator+(const tvec3<T>& l, T r) { return{ l.x + r, l.y + r, l.z + r }; }
template<class T> inline tvec3<T> operator-(const tvec3<T>& l, T r) { return{ l.x - r, l.y - r, l.z - r }; }
template<class T> inline tvec3<T> operator*(const tvec3<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r }; }
template<class T> inline tvec3<T> operator/(const tvec3<T>& l, T r) { return{ l.x / r, l.y / r, l.z / r }; }
template<class T, class U> inline tvec3<T>& operator+=(tvec3<T>& l, const tvec3<U>& r) { l.x += r.x; l.y += r.y; l.z += r.z; return l; }
template<class T, class U> inline tvec3<T>& operator-=(tvec3<T>& l, const tvec3<U>& r) { l.x -= r.x; l.y -= r.y; l.z -= r.z; return l; }
template<class T, class U> inline tvec3<T>& operator*=(tvec3<T>& l, const tvec3<U>& r) { l.x *= r.x; l.y *= r.y; l.z *= r.z; return l; }
template<class T, class U> inline tvec3<T>& operator/=(tvec3<T>& l, const tvec3<U>& r) { l.x /= r.x; l.y /= r.y; l.z /= r.z; return l; }
template<class T> inline tvec3<T>& operator+=(tvec3<T>& l, T r) { l.x += r; l.y += r; l.z += r; return l; }
template<class T> inline tvec3<T>& operator-=(tvec3<T>& l, T r) { l.x -= r; l.y -= r; l.z -= r; return l; }
template<class T> inline tvec3<T>& operator*=(tvec3<T>& l, T r) { l.x *= r; l.y *= r; l.z *= r; return l; }
template<class T> inline tvec3<T>& operator/=(tvec3<T>& l, T r) { l.x /= r; l.y /= r; l.z /= r; return l; }

template<class T> inline tvec4<T> operator-(const tvec4<T>& v) { return{ -v.x, -v.y, -v.z, -v.w }; }
template<class T, class U> inline tvec4<T> operator+(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w }; }
template<class T, class U> inline tvec4<T> operator-(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w }; }
template<class T, class U> inline tvec4<T> operator*(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w }; }
template<class T, class U> inline tvec4<T> operator/(const tvec4<T>& l, const tvec4<U>& r) { return{ l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w }; }
template<class T> inline tvec4<T> operator+(T l, const tvec4<T>& r) { return{ l + r.x, l + r.y, l + r.z, l + r.w }; }
template<class T> inline tvec4<T> operator-(T l, const tvec4<T>& r) { return{ l - r.x, l - r.y, l - r.z, l - r.w }; }
template<class T> inline tvec4<T> operator*(T l, const tvec4<T>& r) { return{ l * r.x, l * r.y, l * r.z, l * r.w }; }
template<class T> inline tvec4<T> operator/(T l, const tvec4<T>& r) { return{ l / r.x, l / r.y, l / r.z, l / r.w }; }
template<class T> inline tvec4<T> operator+(const tvec4<T>& l, T r) { return{ l.x + r, l.y + r, l.z + r, l.w + r }; }
template<class T> inline tvec4<T> operator-(const tvec4<T>& l, T r) { return{ l.x - r, l.y - r, l.z - r, l.w - r }; }
template<class T> inline tvec4<T> operator*(const tvec4<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r, l.w * r }; }
template<class T> inline tvec4<T> operator/(const tvec4<T>& l, T r) { return{ l.x / r, l.y / r, l.z / r, l.w / r }; }
template<class T, class U> inline tvec4<T>& operator+=(tvec4<T>& l, const tvec4<U>& r) { l.x += r.x; l.y += r.y; l.z += r.z; l.w += r.w; return l; }
template<class T, class U> inline tvec4<T>& operator-=(tvec4<T>& l, const tvec4<U>& r) { l.x -= r.x; l.y -= r.y; l.z -= r.z; l.w -= r.w; return l; }
template<class T, class U> inline tvec4<T>& operator*=(tvec4<T>& l, const tvec4<U>& r) { l.x *= r.x; l.y *= r.y; l.z *= r.z; l.w *= r.w; return l; }
template<class T, class U> inline tvec4<T>& operator/=(tvec4<T>& l, const tvec4<U>& r) { l.x /= r.x; l.y /= r.y; l.z /= r.z; l.w /= r.w; return l; }
template<class T> inline tvec4<T>& operator+=(tvec4<T>& l, T r) { l.x += r; l.y += r; l.z += r; l.w += r; return l; }
template<class T> inline tvec4<T>& operator-=(tvec4<T>& l, T r) { l.x -= r; l.y -= r; l.z -= r; l.w -= r; return l; }
template<class T> inline tvec4<T>& operator*=(tvec4<T>& l, T r) { l.x *= r; l.y *= r; l.z *= r; l.w *= r; return l; }
template<class T> inline tvec4<T>& operator/=(tvec4<T>& l, T r) { l.x /= r; l.y /= r; l.z /= r; l.w /= r; return l; }

template<class T> inline tquat<T> operator*(const tquat<T>& l, T r) { return{ l.x * r, l.y * r, l.z * r, l.w * r }; }
template<class T> inline tquat<T> operator*(const tquat<T>& l, const tquat<T>& r)
{
    return{
        l.w * r.x + l.x * r.w + l.y * r.z - l.z * r.y,
        l.w * r.y + l.y * r.w + l.z * r.x - l.x * r.z,
        l.w * r.z + l.z * r.w + l.x * r.y - l.y * r.x,
        l.w * r.w - l.x * r.x - l.y * r.y - l.z * r.z,
    };
}
template<class T> inline tquat<T>& operator*=(tquat<T>& l, T r)
{
    l = l * r;
    return l;
}
template<class T> inline tquat<T>& operator*=(tquat<T>& l, const tquat<T>& r)
{
    l = l * r;
    return l;
}

template<class T> inline T dot(const tvec2<T>& l, const tvec2<T>& r) { return l.x * r.x + l.y * r.y; }
template<class T> inline T dot(const tvec3<T>& l, const tvec3<T>& r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
template<class T> inline T dot(const tvec4<T>& l, const tvec4<T>& r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
template<class T> inline T dot(const tquat<T>& l, const tquat<T>& r) { return dot((const tvec4<T>&)l, (const tvec4<T>&)r); }
template<class T> inline T length_sq(const tvec2<T>& v) { return dot(v, v); }
template<class T> inline T length_sq(const tvec3<T>& v) { return dot(v, v); }
template<class T> inline T length_sq(const tvec4<T>& v) { return dot(v, v); }
template<class T> inline T length(const tvec2<T>& v) { return sqrt(length_sq(v)); }
template<class T> inline T length(const tvec3<T>& v) { return sqrt(length_sq(v)); }
template<class T> inline T length(const tvec4<T>& v) { return sqrt(length_sq(v)); }
template<class T> inline tvec2<T> normalize(const tvec2<T>& v) { return v / length(v); }
template<class T> inline tvec3<T> normalize(const tvec3<T>& v) { return v / length(v); }
template<class T> inline tvec4<T> normalize(const tvec4<T>& v) { return v / length(v); }
template<class T> inline tquat<T> normalize(const tquat<T>& v)
{
    auto r = normalize((const tvec4<T>&)v);
    return (const tquat<T>&)r;
}
template<class T> inline tvec3<T> cross(const tvec3<T>& l, const tvec3<T>& r)
{
    return{
        l.y * r.z - l.z * r.y,
        l.z * r.x - l.x * r.z,
        l.x * r.y - l.y * r.x };
}

template<class T> inline T sum(const tvec2<T>& v) { return v.x + v.y; }
template<class T> inline T sum(const tvec3<T>& v) { return v.x + v.y + v.z; }
template<class T> inline T sum(const tvec4<T>& v) { return v.x + v.y + v.z + v.w; }

template<class T> inline tquat<T> rotate_x(T angle)
{
    T c = std::cos(angle * T(0.5));
    T s = std::sin(angle * T(0.5));
    return{ s, T(0.0), T(0.0), c };
}
template<class T> inline tquat<T> rotate_y(T angle)
{
    T c = std::cos(angle * T(0.5));
    T s = std::sin(angle * T(0.5));
    return{ T(0.0), s, T(0.0), c };
}
template<class T> inline tquat<T> rotate_z(T angle)
{
    T c = std::cos(angle * T(0.5));
    T s = std::sin(angle * T(0.5));
    return{ T(0.0), T(0.0), s, c };
}

template<class T> inline tmat4x4<T> rotate44_x(T angle)
{
    T c = std::cos(angle);
    T s = std::sin(angle);
    return tmat4x4<T> { {
        { 1, 0, 0, 0 },
        { 0, c,-s, 0 },
        { 0, s, c, 0 },
        { 0, 0, 0, 1 },
    }};
}
template<class T> inline tmat4x4<T> rotate44_y(T angle)
{
    T c = std::cos(angle);
    T s = std::sin(angle);
    return tmat4x4<T> { {
        { c, 0, s, 0 },
        { 0, 1, 0, 0 },
        {-s, 0, c, 0 },
        { 0, 0, 0, 1 },
    }};
}
template<class T> inline tmat4x4<T> rotate44_z(T angle)
{
    T c = std::cos(angle);
    T s = std::sin(angle);
    return tmat4x4<T> { {
        { c,-s, 0, 0 },
        { s, c, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 },
    }};
}

template<class T> inline tquat<T> rotate_euler(RotationOrder order, const tvec3<T>& euler)
{
    auto rx = rotate_x(euler.x);
    auto ry = rotate_y(euler.y);
    auto rz = rotate_z(euler.z);
    switch (order)
    {
    case RotationOrder::XYZ: return rz * ry * rx;
    case RotationOrder::XZY: return ry * rz * rx;
    case RotationOrder::YZX: return rx * rz * ry;
    case RotationOrder::YXZ: return rz * rx * ry;
    case RotationOrder::ZXY: return ry * rx * rz;
    case RotationOrder::ZYX: return rx * ry * rz;
    case RotationOrder::SphericXYZ: return rz * ry * rx; // todo
    default: return tquat<T>::identity();
    }
}

template<class T> inline tmat4x4<T> to_mat4x4(const tquat<T>& v)
{
    return tmat4x4<T>{{
        {T(1.0)-T(2.0)*v.y*v.y - T(2.0)*v.z*v.z,T(2.0)*v.x*v.y - T(2.0)*v.z*v.w,         T(2.0)*v.x*v.z + T(2.0)*v.y*v.w,         T(0.0)},
        {T(2.0)*v.x*v.y + T(2.0)*v.z*v.w,       T(1.0) - T(2.0)*v.x*v.x - T(2.0)*v.z*v.z,T(2.0)*v.y*v.z - T(2.0)*v.x*v.w,         T(0.0)},
        {T(2.0)*v.x*v.z - T(2.0)*v.y*v.w,       T(2.0)*v.y*v.z + T(2.0)*v.x*v.w,         T(1.0) - T(2.0)*v.x*v.x - T(2.0)*v.y*v.y,T(0.0)},
        {T(0.0),                                T(0.0),                                  T(0.0),                                  T(1.0)}
    }};
}

template<class T> inline tmat4x4<T> translate(const tvec3<T>& v)
{
    return tmat4x4<T>{{
        {T(1.0), T(0.0), T(0.0), T(0.0)},
        {T(0.0), T(1.0), T(0.0), T(0.0)},
        {T(0.0), T(0.0), T(1.0), T(0.0)},
        {   v.x,    v.y,    v.z, T(1.0)}
    }};
}

template<class T> inline tmat4x4<T> scale44(const tvec3<T>& v)
{
    return tmat4x4<T>{{
        {   v.x, T(0.0), T(0.0), T(0.0)},
        {T(0.0),    v.y, T(0.0), T(0.0)},
        {T(0.0), T(0.0),    v.z, T(0.0)},
        {T(0.0), T(0.0), T(0.0), T(1.0)},
    }};
}

template<class T> inline tmat4x4<T> transform(const tvec3<T>& t, const tquat<T>& r, const tvec3<T>& s)
{
    auto ret = scale44(s);
    ret *= to_mat4x4(r);
    (tvec3<T>&)ret[3] = t;
    return ret;
}

template<class T> inline tmat4x4<T> transpose(const tmat4x4<T>& v)
{
    return tmat4x4<T>{{
        {v[0][0], v[1][0], v[2][0], v[3][0]},
        {v[0][1], v[1][1], v[2][1], v[3][1]},
        {v[0][2], v[1][2], v[2][2], v[3][2]},
        {v[0][3], v[1][3], v[2][3], v[3][3]},
    }};
}

template<class T> inline tmat4x4<T> operator*(const tmat4x4<T>& a, const tmat4x4<T>& b_)
{
    const auto b = transpose(b_);
    return tmat4x4<T>{{
        { sum(a[0] * b[0]), sum(a[0] * b[1]), sum(a[0] * b[2]), sum(a[0] * b[3]) },
        { sum(a[1] * b[0]), sum(a[1] * b[1]), sum(a[1] * b[2]), sum(a[1] * b[3]) },
        { sum(a[2] * b[0]), sum(a[2] * b[1]), sum(a[2] * b[2]), sum(a[2] * b[3]) },
        { sum(a[3] * b[0]), sum(a[3] * b[1]), sum(a[3] * b[2]), sum(a[3] * b[3]) },
    }};
}
template<class T> inline tmat4x4<T>& operator*=(tmat4x4<T>& a, const tmat4x4<T>& b)
{
    a = a * b;
    return a;
}

template<class T>
inline static tvec3<T> mul_v(const tmat4x4<T>& m, const tvec3<T>& v)
{
    return {
        m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2],
        m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2],
        m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2],
    };
}

template<class T>
inline static tvec3<T> mul_p(const tmat4x4<T>& m, const tvec3<T>& v)
{
    return {
        m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0],
        m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1],
        m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2],
    };
}

template<class T> inline tmat4x4<T> invert(const tmat4x4<T>& x)
{
    tmat4x4<T> s = {
        x[1][1] * x[2][2] - x[2][1] * x[1][2],
        x[2][1] * x[0][2] - x[0][1] * x[2][2],
        x[0][1] * x[1][2] - x[1][1] * x[0][2],
        0,

        x[2][0] * x[1][2] - x[1][0] * x[2][2],
        x[0][0] * x[2][2] - x[2][0] * x[0][2],
        x[1][0] * x[0][2] - x[0][0] * x[1][2],
        0,

        x[1][0] * x[2][1] - x[2][0] * x[1][1],
        x[2][0] * x[0][1] - x[0][0] * x[2][1],
        x[0][0] * x[1][1] - x[1][0] * x[0][1],
        0,

        0, 0, 0, 1,
    };

    auto r = x[0][0] * s[0][0] + x[0][1] * s[1][0] + x[0][2] * s[2][0];

    if (abs(r) >= 1) {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                s[i][j] /= r;
            }
        }
    }
    else {
        auto mr = abs(r) / std::numeric_limits<T>::min();

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (mr > abs(s[i][j])) {
                    s[i][j] /= r;
                }
                else {
                    // error
                    return tmat4x4<T>::identity();
                }
            }
        }
    }

    s[3][0] = -x[3][0] * s[0][0] - x[3][1] * s[1][0] - x[3][2] * s[2][0];
    s[3][1] = -x[3][0] * s[0][1] - x[3][1] * s[1][1] - x[3][2] * s[2][1];
    s[3][2] = -x[3][0] * s[0][2] - x[3][1] * s[1][2] - x[3][2] * s[2][2];
    return s;
}

} // namespace sfbx
