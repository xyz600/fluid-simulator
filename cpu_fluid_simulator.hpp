#pragma once

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>

#include "problem.hpp"

using Color = Vec<std::uint8_t, 4>;

template <typename T>
class UpwindDifferenceCalculator {
public:
    UpwindDifferenceCalculator(const T* buffer, Vec2f* velocity, std::size_t height, std::size_t width)
        : m_height_(height)
        , m_width_(width)
        , m_buffer_(buffer)
        , m_velocity_(velocity)
    {
    }

    std::pair<T, T> first_order_diff(const std::size_t y, const std::size_t x) const
    {
        assert(1 <= x && x < m_width_ - 1 && 1 <= y && y < m_height_ - 1);
        const auto index = y * m_width_ + x;
        // x < 0 なら左に流れるので、右（x + 1）から取ってくる。 x > 0 から逆に x - 1 から取ってくる
        const auto x_val = m_velocity_[index].x() < 0 ? m_buffer_[index + 1] : m_buffer_[index - 1];
        const auto y_val = m_velocity_[index].y() < 0 ? m_buffer_[index + m_width_] : m_buffer_[index - m_width_];

        return std::make_pair(x_val, y_val);
    }

    std::pair<T, T> second_order_diff(const std::size_t y, const std::size_t x) const
    {
        assert(1 <= x && x < m_width_ - 1 && 1 <= y && y < m_height_ - 1);

        const auto index = y * m_width_ + x;
        const auto x_val = m_buffer_[index + 1] + m_buffer_[index - 1] - 2 * m_buffer_[index];
        const auto y_val = m_buffer_[index + m_width_] + m_buffer_[index - m_width_] - 2 * m_buffer_[index];

        return std::make_pair(x_val, y_val);
    }

private:
    T* m_buffer_;

    Vec2f* m_velocity_;

    std::size_t m_width_;

    std::size_t m_height_;
};

class CPUFluidSimulator {
public:
    CPUFluidSimulator(const std::size_t width, const std::size_t height, GLuint pbo)
        : width_(width)
        , height_(height)
        , pbo_(pbo)
        , buffer_(new Color[height * width])
        , velocity_(new Vec2f[height * width])
        , prev_velocity_(new Vec2f[height * width])
        , pressure_(new Vec2f[height * width])
        , prev_pressure_(new Vec2f[height * width])
        , fixed_(new bool[height * width])
    {
    }

    void update()
    {
        // update v
        velocity_.swap(prev_velocity_);

        // update p
        pressure_.swap(prev_pressure_);
    }

    void draw_background()
    {
        // 雑だけど、 glDrawPixels() を呼ぶところまでやる

        update();

        for (std::size_t i = 0; i < height_; i++) {
            for (std::size_t j = 0; j < width_; j++) {
                const std::uint8_t val = ((i + j) * 10) % 255;
                buffer_[i * width_ + j].x() = val;
                buffer_[i * width_ + j].y() = val;
                buffer_[i * width_ + j].z() = val;
                buffer_[i * width_ + j].w() = val;
            }
        }
        glDrawPixels(width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer_.get());
    }

private:
    std::size_t width_;

    std::size_t height_;

    GLuint pbo_;

    // color buffer
    std::unique_ptr<Color[]> buffer_;

    std::unique_ptr<Vec2f[]> velocity_;

    std::unique_ptr<Vec2f[]> prev_velocity_;

    std::unique_ptr<Vec2f[]> pressure_;

    std::unique_ptr<Vec2f[]> prev_pressure_;

    std::unique_ptr<Color[]> color_;

    std::unique_ptr<Color[]> prev_color_;

    std::unique_ptr<bool[]> fixed_;
};