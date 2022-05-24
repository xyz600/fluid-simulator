#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

struct Problem {
    std::size_t width;

    std::size_t height;
};

template <typename T, std::size_t N>
class Vec {
public:
    using value_t = T;

    Vec()
    {
        m_data_.fill(0);
    }

    void fill(const T val)
    {
        m_data_.fill(val);
    }

    Vec(std::initializer_list<T> init)
    {
        std::size_t index = 0;
        for (auto v : init) {
            m_data_[index++] = v;
        }
    }

    T operator[](const std::size_t index) const
    {
        return m_data_[index];
    }

    T& operator[](const std::size_t index)
    {
        return m_data_[index];
    }

    T x() const
    {
        assert(N >= 1);
        return m_data_[0];
    }

    T& x()
    {
        assert(N >= 1);
        return m_data_[0];
    }

    T y() const
    {
        assert(N >= 2);
        return m_data_[1];
    }

    T& y()
    {
        assert(N >= 2);
        return m_data_[1];
    }

    T z() const
    {
        assert(N >= 3);
        return m_data_[2];
    }

    T& z()
    {
        assert(N >= 3);
        return m_data_[2];
    }

    T w() const
    {
        assert(N >= 4);
        return m_data_[3];
    }

    T& w()
    {
        assert(N >= 4);
        return m_data_[3];
    }

    // 手抜き
    auto begin() const
    {
        return m_data_.begin();
    }

    auto cbegin() const
    {
        return m_data_.cbegin();
    }

    auto end() const
    {
        return m_data_.end();
    }

    auto cend() const
    {
        return m_data_.cend();
    }

private:
    std::array<T, N> m_data_;
};

template <typename T>
using Vec2 = Vec<T, 2>;

using Vec2f = Vec<float, 2>;
using Vec2d = Vec<double, 2>;

template <typename T>
using Vec3 = Vec<T, 3>;

using Vec3f = Vec<float, 3>;
using Vec3d = Vec<double, 3>;

template <typename T, std::size_t N>
std::ostream& operator<<(std::ostream& out, const Vec<T, N>& vec)
{
    out << "[";
    bool first = true;
    for (const auto v : vec) {
        if (first) {
            first = false;
        } else {
            out << ", ";
        }
        out << v;
    }
    out << "]";
    return out;
}

template <typename T, std::size_t N>
Vec<T, N> operator+(const Vec<T, N> v1, const Vec<T, N> v2)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = v1[i] + v2[i];
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> operator-(const Vec<T, N> v1, const Vec<T, N> v2)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = v1[i] - v2[i];
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> operator*(const Vec<T, N> v, const T coef)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = v[i] * coef;
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> operator/(const Vec<T, N> v, const T coef)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = v[i] / coef;
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> operator-(const Vec<T, N>& v)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = -v[i];
    }
    return ret;
}

template <typename T, std::size_t N>
T dot(const Vec<T, N>& v1, const Vec<T, N>& v2)
{
    T ret = 0;
    for (std::size_t i = 0; i < N; i++) {
        ret += v1[i] * v2[i];
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> max(const Vec<T, N>& v1, const Vec<T, N>& v2)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = std::max(v1[i], v2[i]);
    }
    return ret;
}

template <typename T, std::size_t N>
Vec<T, N> min(const Vec<T, N>& v1, const Vec<T, N>& v2)
{
    Vec<T, N> ret;
    for (std::size_t i = 0; i < N; i++) {
        ret[i] = std::min(v1[i], v2[i]);
    }
    return ret;
}