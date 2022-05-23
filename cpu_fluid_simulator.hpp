#pragma once

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>

struct Color {
    std::uint8_t x;
    std::uint8_t y;
    std::uint8_t z;
    std::uint8_t w;
};

class CPUFluidSimulator {
public:
    CPUFluidSimulator(const std::size_t width, const std::size_t height, GLuint pbo)
        : width_(width)
        , height_(height)
        , pbo_(pbo)
        , buffer_(new Color[height * width])
        , velocity_(new float[height * width])
        , prev_velocity_(new float[height * width])
        , pressure_(new float[height * width])
        , prev_pressure_(new float[height * width])
    {
    }

    void draw_background()
    {
        // 雑だけど、 glDrawPixels() を呼ぶところまでやる
        for (std::size_t i = 0; i < height_; i++) {
            for (std::size_t j = 0; j < width_; j++) {
                const std::uint8_t val = ((i + j) * 10) % 255;
                buffer_[i * width_ + j].x = val;
                buffer_[i * width_ + j].y = val;
                buffer_[i * width_ + j].z = val;
                buffer_[i * width_ + j].w = val;
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

    std::unique_ptr<float[]> velocity_;

    std::unique_ptr<float[]> prev_velocity_;

    std::unique_ptr<float[]> pressure_;

    std::unique_ptr<float[]> prev_pressure_;
};