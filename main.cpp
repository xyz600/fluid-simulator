// based on https://gist.github.com/kamino410/09df4ecdf37b03cbd05752a7b2e52d3a
// this adds ImGui to an application with CUDA and OpenGL. the thing is, once you use CUDA, ImGui renders very strangely.
// after 9 hours of debugging I found that putting glBindBuffer before and after the draw call fixes this!
// glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo); // THE MAGIC LINE #1
// glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
// glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);   // THE MAGIC LINE #2

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>

#include "cpu_fluid_simulator.hpp"
#include "problem.hpp"

struct ViewerState {
    bool show_config;
    ImVec4 clear_color;

    PrintType type;
    int type_int;

    ViewerState()
        : show_config(true)
        , clear_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
        , type(PrintType::VELOCITY)
        , type_int(0)
    {
    }
};

int main()
{
    // GLFW + OpenGL
    if (!glfwInit())
        exit(EXIT_FAILURE);

    if (atexit(glfwTerminate)) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwSetErrorCallback([](int error, const char* msg) {
        std::cerr << msg << std::endl;
    });

    constexpr std::size_t WIDTH = 200;
    constexpr std::size_t HEIGHT = 200;

    GLuint pbo;

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Simulator", nullptr, nullptr);

    if (!window)
        exit(EXIT_FAILURE);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark(); // ImGui::StyleColorsClassic();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ViewerState state;

    // CUDA with GL interop
    glGenBuffers(1, &pbo); // make & register PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 4 * sizeof(GLubyte) * WIDTH * HEIGHT, NULL, GL_DYNAMIC_DRAW);

    auto config = Config(HEIGHT, WIDTH);
    config.Re = 1e2;
    config.dt = 1e-1;
    config.pbo = pbo;

    for (std::size_t y = 0; y < HEIGHT; y++) {
        config.set_fixed(y, 0);
        config.set_fixed(y, WIDTH - 1);
    }
    for (std::size_t x = 0; x < WIDTH; x++) {
        config.set_fixed(0, x);
        config.set_fixed(HEIGHT - 1, x);
    }

    for (std::size_t y = 90; y <= 110; y++) {
        for (std::size_t x = 90; x <= 110; x++) {
            config.set_fixed(y, x);
        }
    }

    auto simulator = CPUFluidSimulator(std::move(config));

    std::size_t turn = 0;

    while (!glfwWindowShouldClose(window)) {

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        simulator.draw_background(state.type);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space))) {
            break;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab))) {
            state.show_config = !state.show_config;
        }

        // 設定周り色々
        if (state.show_config) {
            ImGui::Begin("Config");

            ImGui::RadioButton("velocity", &state.type_int, 0);
            ImGui::SameLine();
            ImGui::RadioButton("pressure", &state.type_int, 1);
            ImGui::SameLine();
            ImGui::RadioButton("fixed", &state.type_int, 2);

            switch (state.type_int) {
            case 0:
                state.type = PrintType::VELOCITY;
                break;
            case 1:
                state.type = PrintType::PRESSURE;
                break;
            case 2:
                state.type = PrintType::FIXED;
                break;
            }

            const auto pos = ImGui::GetMousePos();

            const auto rel_x = std::round(pos.x);
            const auto rel_y = HEIGHT - std::round(pos.y);

            ImGui::Text("(rx, ry) = (%.3f, %.3f)", rel_x, rel_y);
            if (0 <= rel_x && rel_x < WIDTH && 0 <= rel_y && rel_y < HEIGHT) {
                const auto vec = simulator.Velocity(rel_y, rel_x);
                ImGui::Text("(vx, vy) = (%.3f, %.3f)", vec.x(), vec.y());
            } else {
                ImGui::Text("(vx, vy) = ");
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        turn++;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); // exit
    glDeleteBuffers(1, &pbo);
    glfwTerminate();
    return 0;
}
