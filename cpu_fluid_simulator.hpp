#pragma once

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>

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
        , buffer_(height * width * 4)
        , frame_(0)
    {
    }

    void initialize()
    {
    }

    void draw_background()
    {
        frame_++;

        // 雑だけど、 glDrawPixels() を呼ぶところまでやる
        for (std::size_t i = 0; i < height_; i++) {
            for (std::size_t j = 0; j < width_; j++) {
                const std::uint8_t val = ((i + j) * frame_ / 10) % 255;
                buffer_[i * width_ + j].x = val;
                buffer_[i * width_ + j].y = val;
                buffer_[i * width_ + j].z = val;
                buffer_[i * width_ + j].w = val;
            }
        }
        glDrawPixels(width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buffer_.data());
    }

private:
    std::size_t width_;

    std::size_t height_;

    GLuint pbo_;

    std::vector<Color> buffer_;

    std::size_t frame_;
};