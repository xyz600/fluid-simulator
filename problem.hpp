#pragma once

#include <cstdint>

struct Problem {
    std::size_t width;

    std::size_t height;
};

struct Vec2 {
    float x;
    float y;
};

struct State {
    Vec2 velocity;
};