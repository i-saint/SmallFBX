#pragma once

#include "sfbxTypes.h"

namespace sfbx {

// simpler version of std::vector.
// T must be POD types because its constructor and destructor are never called.
// that also means this can be significantly faster than std::vector in some specific situations.
// (e.g. temporary buffers that can be very large and frequently resized)
template<class T>
class RawVector
{
public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    RawVector() {}
    RawVector(RawVector&& v) noexcept { swap(v); }
    RawVector(const RawVector& v) { assign(v.begin(), v.end()); }
    RawVector(std::initializer_list<T> v) { assign(v); }
    RawVector(const_iterator b, const_iterator e) { assign(b, e); }
    template<class U> RawVector(span<U> v) { assign(v); }
    template<class U, size_t N> RawVector(U(&v)[N]) { assign(v); }
    explicit RawVector(size_t initial_size) { resize(initial_size); }

    void operator=(RawVector&& v) noexcept { swap(v); }
    void operator=(const RawVector& v) { assign(v.begin(), v.end()); }
    void operator=(std::initializer_list<T> v) { assign(v); }
    template<class U> void operator=(span<U> v) { assign(v); }
    template<class U, size_t N> void operator=(U (&v)[N]) { assign(v, v + N); }

    ~RawVector()
    {
        clear();
        shrink_to_fit();
    }

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }
    size_t capacity() const { return m_capacity; }
    size_t size_bytes() const { return sizeof(T) * m_size; }
    size_t capacity_bytes() const { return sizeof(T) * m_capacity; }

    T* data() { return m_data; }
    const T* data() const { return m_data; }

    T& at(size_t i) { return m_data[i]; }
    const T& at(size_t i) const { return m_data[i]; }
    T& operator[](size_t i) { return m_data[i]; }
    const T& operator[](size_t i) const { return m_data[i]; }

    T& front() { return m_data[0]; }
    T& back() { return m_data[m_size - 1]; }
    const T& front() const { return m_data[0]; }
    const T& back() const { return m_data[m_size - 1]; }

    iterator begin() { return m_data; }
    iterator end() { return m_data + m_size; }
    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_size; }

    static T* allocate(size_t size) { return (T*)malloc(size); }
    static void deallocate(T* addr, size_t /*size*/) { free(addr); }

    void reserve(size_t s)
    {
        if (s > m_capacity) {
            s = std::max<size_t>(s, m_size * 2);
            size_t newsize = sizeof(T) * s;
            size_t oldsize = sizeof(T) * m_size;

            T* newdata = allocate(newsize);
            memcpy(newdata, m_data, oldsize);
            deallocate(m_data, oldsize);
            m_data = newdata;
            m_capacity = s;
        }
    }

    void shrink_to_fit()
    {
        if (m_size == 0) {
            deallocate(m_data, m_size);
            m_data = nullptr;
            m_size = m_capacity = 0;
        }
        else if (m_size == m_capacity) {
            // nothing to do
            return;
        }
        else {
            size_t newsize = sizeof(T) * m_size;
            size_t oldsize = sizeof(T) * m_capacity;
            T* newdata = allocate(newsize);
            memcpy(newdata, m_data, newsize);
            deallocate(m_data, oldsize);
            m_data = newdata;
            m_capacity = m_size;
        }
    }

    void resize(size_t s)
    {
        reserve(s);
        m_size = s;
    }
    void resize(size_t s, const T& v)
    {
        size_t pos = size();
        resize(s);
        // std::fill() can be significantly slower than plain copy
        for (size_t i = pos; i < s; ++i)
            m_data[i] = v;
    }

    void clear()
    {
        m_size = 0;
    }

    void swap(RawVector& other)
    {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    template<class Iter>
    void assign(Iter first, Iter last)
    {
        resize(std::distance(first, last));
        std::copy(first, last, begin());
    }
    void assign(std::initializer_list<T> v)
    {
        assign(v.begin(), v.end());
    }
    void assign(span<T> v)
    {
        resize(v.size());
        memcpy(data(), v.data(), size_bytes());
    }
    template<class U>
    void assign(span<U> v)
    {
        size_t n = v.size();
        resize(n);
        for (size_t i = 0; i < n; ++i)
            m_data[i] = T(v[i]);
    }
    template<class U>
    void assign(const U* v, size_t n)
    {
        assign(make_span(v, n));
    }

    template<class ForwardIter>
    void insert(iterator pos, ForwardIter first, ForwardIter last)
    {
        size_t d = std::distance(begin(), pos);
        size_t s = std::distance(first, last);
        resize(d + s);
        std::copy(first, last, begin() + d);
    }
    void insert(iterator pos, const_pointer first, const_pointer last)
    {
        size_t d = std::distance(begin(), pos);
        size_t s = std::distance(first, last);
        resize(d + s);
        memcpy(m_data + d, first, sizeof(value_type) * s);
    }
    void insert(iterator pos, const_reference v)
    {
        insert(pos, &v, &v + 1);
    }

    void erase(iterator first, iterator last)
    {
        size_t s = std::distance(first, last);
        std::copy(last, end(), first);
        m_size -= s;
    }

    void erase(iterator pos)
    {
        erase(pos, pos + 1);
    }

    void push_back(const T& v)
    {
        resize(m_size + 1);
        back() = v;
    }
    void push_back(T&& v)
    {
        resize(m_size + 1);
        back() = std::move(v);
    }

    void pop_back()
    {
        if (m_size > 0)
            --m_size;
    }

    bool operator==(const RawVector& v) const
    {
        return m_size == v.m_size && memcmp(m_data, v.m_data, sizeof(T) * m_size) == 0;
    }
    bool operator!=(const RawVector& v) const
    {
        return !(*this == v);
    }

    void zeroclear()
    {
        memset(m_data, 0, sizeof(T) * m_size);
    }

private:
    T* m_data = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;
};

template<class T> inline constexpr bool is_RawVector = false;
template<class T> inline constexpr bool is_RawVector<RawVector<T>> = true;

} // namespace sfbx
