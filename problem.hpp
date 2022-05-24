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

private:
    template <class... Args>
    void inner_assign(const std::size_t index)
    {
    }

    template <class... Args>
    void inner_assign(const std::size_t index, T val, Args... args)
    {
        m_data_[index] = val;
        inner_assign(index + 1, args...);
    }

public:
    template <class... Args>
    Vec(Args... args)
    {
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
    auto begin()
    {
        return m_data_.begin();
    }

    auto end()
    {
        return m_data_.end();
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
    for (auto v : vec) {
        out << v << ", ";
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
Vec<T, N> operator-(const Vec<T, N> v)
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