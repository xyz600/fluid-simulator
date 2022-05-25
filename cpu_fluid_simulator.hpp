#pragma once

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <chrono>
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
        const auto x_val = m_velocity_[index].x() < 0 ? m_buffer_[index + 1] - m_buffer_[index] : m_buffer_[index] - m_buffer_[index - 1];
        const auto y_val = m_velocity_[index].y() < 0 ? m_buffer_[index + m_width_] - m_buffer_[index] : m_buffer_[index] - m_buffer_[index - m_width_];

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
    CPUFluidSimulator(Config&& config)
        : m_width_(config.width)
        , m_height_(config.height)
        , m_velocity_(new Vec2f[config.size()])
        , m_prev_velocity_(new Vec2f[config.size()])
        , m_pressure_(new float[config.size()])
        , m_prev_pressure_(new float[config.size()])
        , m_fixed_(std::move(config.fixed))
        , m_Re_(config.Re)
        , m_dt_(config.dt)
    {
        for (std::size_t y = 0; y < m_height_; y++) {
            const auto index = y * m_width_ + 0;
            m_velocity_[index].x() = 5.0;
            m_prev_velocity_[index].x() = 5.0;
        }
    }

    void update_velocity()
    {
        m_velocity_.swap(m_prev_velocity_);

        const auto velocity_calculator = UpwindDifferenceCalculator<Vec2f>(m_prev_velocity_.get(), m_prev_velocity_.get(), m_height_, m_width_);

        const auto pressure_calculator = UpwindDifferenceCalculator<float>(m_prev_pressure_.get(), m_prev_velocity_.get(), m_height_, m_width_);

        // 境界条件の 1px 内側に関して、全て更新する
#pragma omp parallel for num_threads(10)
        for (std::size_t y = 0; y < m_height_; y++) {
            for (std::size_t x = 0; x < m_width_; x++) {
                const auto index = y * m_width_ + x;

                if (m_fixed_[index]) {
                    continue;
                }

                const auto [dx1, dy1] = velocity_calculator.first_order_diff(y, x);
                const auto term1 = -(dx1 * m_prev_velocity_[index].x() + dy1 * m_prev_velocity_[index].y());

                const auto [dx2, dy2] = pressure_calculator.first_order_diff(y, x);
                const auto term2 = Vec2f({ -dx2, -dy2 });

                const auto [dx3, dy3] = velocity_calculator.second_order_diff(y, x);
                const auto term3 = (dx3 + dy3) / m_Re_;

                m_velocity_[index] = m_prev_velocity_[index] + (term1 + term2 + term3) * m_dt_;
            }
        }
    }

    void update_pressure()
    {
        const auto velocity_calculator = UpwindDifferenceCalculator<Vec2f>(m_velocity_.get(), m_velocity_.get(), m_height_, m_width_);

        constexpr std::size_t MaxIter = 4;
        for (std::size_t i = 0; i < m_height_ * m_width_; i++) {
            m_pressure_[i] = 0;
        }

        for (std::size_t iter = 0; iter < MaxIter; iter++) {
            m_pressure_.swap(m_prev_pressure_);

            float diff = 0;

            // 境界条件の 1px 内側に関して、全て更新する
#pragma omp parallel for num_threads(10)
            for (std::size_t y = 0; y < m_height_; y++) {
                for (std::size_t x = 0; x < m_width_; x++) {
                    const auto index = y * m_width_ + x;

                    if (m_fixed_[index]) {
                        continue;
                    }

                    const auto [dx, dy] = velocity_calculator.first_order_diff(y, x);
                    const auto term = (dx.x() * dx.x() + dy.y() * dy.y() + 2 * dy.x() * dx.y()) / m_Re_;

                    m_pressure_[index] = (m_prev_pressure_[index + 1] + m_prev_pressure_[index - 1] + m_prev_pressure_[index + m_width_] + m_prev_pressure_[index - m_width_] + term) / 4.0f;
                    diff += std::abs(m_pressure_[index] - m_prev_pressure_[index]);
                }
            }
        }
    }

    void update()
    {
        update_velocity();
        update_pressure();
    }

    Vec2f velocity(const std::size_t y, const std::size_t x) const
    {
        return m_velocity_[y * m_width_ + x];
    }

    float pressure(const std::size_t y, const std::size_t x) const
    {
        return m_pressure_[y * m_width_ + x];
    }

    bool fixed(const std::size_t y, const std::size_t x) const
    {
        return m_fixed_[y * m_width_ + x];
    }

private:
    std::size_t m_width_;

    std::size_t m_height_;

    std::unique_ptr<Vec2f[]> m_velocity_;

    std::unique_ptr<Vec2f[]> m_prev_velocity_;

    std::unique_ptr<float[]> m_pressure_;

    std::unique_ptr<float[]> m_prev_pressure_;

    std::unique_ptr<Color[]> m_color_;

    std::unique_ptr<Color[]> m_prev_color_;

    std::unique_ptr<bool[]> m_fixed_;

    // レイノルズ数
    float m_Re_;

    float m_dt_;
};