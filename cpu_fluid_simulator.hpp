#pragma once

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>

#include "problem.hpp"

template <typename T>
class UpwindDifferenceCalculator {
public:
    UpwindDifferenceCalculator(T* buffer, Vec2f* velocity, std::size_t height, std::size_t width)
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
        const auto x_val = m_buffer_[index + 1] + m_buffer_[index - 1] - m_buffer_[index] * 2.0f;
        const auto y_val = m_buffer_[index + m_width_] + m_buffer_[index - m_width_] - m_buffer_[index] * 2.0f;

        return std::make_pair(x_val, y_val);
    }

private:
    T* m_buffer_;

    Vec2f* m_velocity_;

    std::size_t m_width_;

    std::size_t m_height_;
};

using Color = Vec<std::uint8_t, 4>;

class CPUFluidSimulator {
public:
    CPUFluidSimulator(const std::size_t width, const std::size_t height, GLuint pbo, float Re)
        : m_width_(width)
        , m_height_(height)
        , m_pbo_(pbo)
        , m_buffer_(new Color[height * width])
        , m_velocity_(new Vec2f[height * width])
        , m_prev_velocity_(new Vec2f[height * width])
        , m_pressure_(new float[height * width])
        , m_prev_pressure_(new float[height * width])
        , m_fixed_(new bool[height * width])
        , m_Re_(Re)
    {
        for (std::size_t y = 0; y < 100; y++) {
            const auto index = y * m_width_ + 0;
            m_velocity_[index].x() = 5.0;
            m_prev_velocity_[index].x() = 5.0;
        }
    }

    void update_velocity()
    {
        const auto velocity_calculator = UpwindDifferenceCalculator<Vec2f>(m_prev_velocity_.get(), m_prev_velocity_.get(), m_height_, m_width_);

        const auto pressure_calculator = UpwindDifferenceCalculator<float>(m_prev_pressure_.get(), m_prev_velocity_.get(), m_height_, m_width_);

        m_max_velocity_.fill(std::numeric_limits<float>::lowest());
        m_min_velocity_.fill(std::numeric_limits<float>::max());

        // 境界条件の 1px 内側に関して、全て更新する
        for (std::size_t y = 1; y < m_height_ - 1; y++) {
            for (std::size_t x = 1; x < m_width_ - 1; x++) {
                const auto index = y * m_width_ + x;

                const auto [dx1, dy1] = velocity_calculator.first_order_diff(y, x);
                const auto term1 = -(dx1 * m_prev_velocity_[index].x() + dy1 * m_prev_velocity_[index].y());

                const auto [dx2, dy2] = pressure_calculator.first_order_diff(y, x);
                const auto term2 = -Vec2f(dx2, dy2);

                const auto [dx3, dy3] = velocity_calculator.second_order_diff(y, x);
                const auto term3 = (dx3 + dy3) / m_Re_;

                m_velocity_[index] = term1 + term2 + term3;

                m_min_velocity_ = min(m_min_velocity_, m_velocity_[index]);
                m_min_velocity_ = max(m_max_velocity_, m_velocity_[index]);
            }
        }
    }

    void update_pressure()
    {
        const auto velocity_calculator = UpwindDifferenceCalculator<Vec2f>(m_prev_velocity_.get(), m_prev_velocity_.get(), m_height_, m_width_);

        constexpr std::size_t MaxIter = 5;

        for (std::size_t iter = 0; iter < MaxIter; iter++) {
            // 境界条件の 1px 内側に関して、全て更新する
            for (std::size_t y = 1; y < m_height_ - 1; y++) {
                for (std::size_t x = 1; x < m_width_ - 1; x++) {
                    const auto index = y * m_width_ + x;

                    const auto [dx, dy] = velocity_calculator.first_order_diff(y, x);

                    const auto term = (dx.x() * dx.x() + dy.y() * dy.y() + 2 * dy.x() * dx.y()) / m_Re_;

                    m_pressure_[index] = (m_prev_pressure_[index + 1] + m_prev_pressure_[index - 1] + m_prev_pressure_[index + m_width_] + m_prev_pressure_[index - m_width_] - term) / 4.0f;
                }
            }

            if (iter < MaxIter - 1) {
                // 反復法のために swap
                m_pressure_.swap(m_prev_pressure_);
            }
        }
        // m_pressure_ に答えが入っている
    }

    void update()
    {
        m_velocity_.swap(m_prev_velocity_);
        m_pressure_.swap(m_prev_pressure_);

        update_velocity();

        update_pressure();
    }

    void draw_background()
    {
        // 雑だけど、 glDrawPixels() を呼ぶところまでやる
        update();

        for (std::size_t i = 0; i < m_height_; i++) {
            for (std::size_t j = 0; j < m_width_; j++) {
                const auto index = i * m_width_ + j;

                const auto x = static_cast<std::uint8_t>(255.0 * (m_velocity_[index].x() - m_min_velocity_.x()) / (m_max_velocity_.x() - m_min_velocity_.x()));
                const auto y = static_cast<std::uint8_t>(255.0 * (m_velocity_[index].y() - m_min_velocity_.y()) / (m_max_velocity_.y() - m_min_velocity_.y()));

                if (i == 0 && j == 0) {
                    std::cerr << "velocity: " << m_velocity_[0] << " " << m_velocity_[1] << std::endl;
                }

                m_buffer_[index].x() = x;
                m_buffer_[index].y() = y;
                m_buffer_[index].z() = 0;
                m_buffer_[index].w() = 255;
            }
        }
        glDrawPixels(m_width_, m_height_, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer_.get());
    }

private:
    std::size_t m_width_;

    std::size_t m_height_;

    GLuint m_pbo_;

    // color buffer
    std::unique_ptr<Color[]> m_buffer_;

    std::unique_ptr<Vec2f[]> m_velocity_;

    std::unique_ptr<Vec2f[]> m_prev_velocity_;

    std::unique_ptr<float[]> m_pressure_;

    std::unique_ptr<float[]> m_prev_pressure_;

    std::unique_ptr<Color[]> m_color_;

    std::unique_ptr<Color[]> m_prev_color_;

    std::unique_ptr<bool[]> m_fixed_;

    Vec2f m_min_velocity_;
    Vec2f m_max_velocity_;

    // レイノルズ数
    float m_Re_;
};