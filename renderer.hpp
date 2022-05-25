#pragma once

#include "cpu_fluid_simulator.hpp"
#include "problem.hpp"

struct RenderConfig {
    // [-range, range] -> [0, 255] に map する
    float pressure_range;
    // [-range, range] -> [0, 255] に map する
    float velocity_range;
    PrintType type;

    RenderConfig()
        : pressure_range(1.0)
        , velocity_range(1.0)
    {
    }
};

class Renderer {
public:
    Renderer(const std::size_t sim_height, const std::size_t sim_width, const std::size_t scale, const GLuint pbo)
        : m_height_(sim_height)
        , m_width_(sim_width)
        , m_scale_(scale)
        , m_buffer_(new Color[sim_height * scale * sim_width * scale])
        , m_pbo_(pbo)
    {
    }

    void doit(const CPUFluidSimulator& simulator, const RenderConfig config)
    {
#pragma omp parallel for num_threads(10)
        for (std::size_t i = 0; i < m_height_; i++) {
            for (std::size_t j = 0; j < m_width_; j++) {
                const auto color = color_of(i, j, simulator, config);
                for (std::size_t ii = i * m_scale_; ii < (i + 1) * m_scale_; ii++) {
                    for (std::size_t jj = j * m_scale_; jj < (j + 1) * m_scale_; jj++) {
                        m_buffer_[to_index(ii, jj)] = color;
                    }
                }
            }
        }
        glDrawPixels(m_width_ * m_scale_, m_height_ * m_scale_, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer_.get());
    }

private:
    std::size_t to_index(const std::size_t i, const std::size_t j)
    {
        return i * m_width_ * m_scale_ + j;
    }

    std::uint8_t to_color_scale(float range, float val) const
    {
        // [-range, range] -> [0, 255] に変換
        return static_cast<std::uint8_t>((val / (2.0 * range) + 0.5) * 255.0);
    }

    Color color_of(const std::size_t sim_y, const std::size_t sim_x, const CPUFluidSimulator& simulator, const RenderConfig& config) const
    {
        if (config.type == PrintType::PRESSURE) {
            const auto p = simulator.pressure(sim_y, sim_x);
            const auto val = to_color_scale(config.pressure_range, p);
            return Color { val, val, val, 255 };

        } else if (config.type == PrintType::VELOCITY) {
            const auto v = simulator.velocity(sim_y, sim_x);
            return Color {
                to_color_scale(config.velocity_range, v.x()),
                to_color_scale(config.velocity_range, v.y()),
                0, 255
            };
        } else {
            const auto fixed = simulator.fixed(sim_y, sim_x);
            const auto val = static_cast<std::uint8_t>(fixed * 255);
            return Color { val, val, val, 255 };
        }
    }

    GLuint m_pbo_;

    std::unique_ptr<Color[]> m_buffer_;

    std::size_t m_width_;

    std::size_t m_height_;

    std::size_t m_scale_;
};