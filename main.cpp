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
#include "renderer.hpp"

struct ViewerState {
    bool show_config;
    ImVec4 clear_color;

    PrintType type;
    int type_int;

    bool stopped;

    ViewerState()
        : show_config(true)
        , clear_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
        , type(PrintType::VELOCITY)
        , type_int(0)
        , stopped(false)
    {
    }
};

int main()
{
    // config
    constexpr std::size_t WIDTH = 200;
    constexpr std::size_t HEIGHT = 200;

    constexpr std::size_t SCALE = 6;

    constexpr std::size_t WINDOW_WIDTH = WIDTH * SCALE;
    constexpr std::size_t WINDOW_HEIGHT = HEIGHT * SCALE;

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

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fluid Simulator", nullptr, nullptr);

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

    GLuint pbo;

    // CUDA with GL interop
    glGenBuffers(1, &pbo); // make & register PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 4 * sizeof(GLubyte) * WINDOW_WIDTH * WINDOW_HEIGHT, NULL, GL_DYNAMIC_DRAW);

    ViewerState state;

    auto config = Config(HEIGHT, WIDTH);
    config.Re = 1e0;
    config.dt = 1e-1;
    config.pbo = pbo;
    config.scale = SCALE;

    for (std::size_t y = 0; y < HEIGHT; y++) {
        config.set_fixed(y, 0);
        config.set_fixed(y, WIDTH - 1);
    }
    for (std::size_t x = 0; x < WIDTH; x++) {
        config.set_fixed(0, x);
        config.set_fixed(HEIGHT - 1, x);
    }

    for (std::size_t y = 0; y < HEIGHT; y++) {
        for (std::size_t x = 0; x < WIDTH; x++) {
            const auto dy = (y - HEIGHT / 2);
            const auto dx = (x - WIDTH / 2);
            const auto dist2 = dy * dy + dx * dx;
            if (dist2 < 100) {
                config.set_fixed(y, x);
            }
        }
    }

    auto simulator = CPUFluidSimulator(config);

    auto renderer = Renderer(HEIGHT, WIDTH, SCALE, pbo);

    auto render_config = RenderConfig();

    // vis
    std::size_t turn = 0;

    while (!glfwWindowShouldClose(window)) {

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        if (!state.stopped) {

            constexpr std::size_t MAX_ITER = 3;

            const auto start = std::chrono::high_resolution_clock::now();
            for (std::size_t iter = 0; iter < MAX_ITER; iter++) {
                simulator.update();
            }
            const auto end = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            std::cerr << "elapsed: " << elapsed << "[ms]" << std::endl;
        }
        render_config.type = state.type;
        // draw background
        renderer.doit(simulator, render_config);

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Space))) {
            break;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab))) {
            state.show_config = !state.show_config;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A))) {
            state.stopped = !state.stopped;
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
            simulator.initialize(config);
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

            const auto rel_x = std::round(pos.x / config.scale);
            const auto rel_y = HEIGHT - std::round(pos.y / config.scale);

            ImGui::Text("(rx, ry) = (%.3f, %.3f)", rel_x, rel_y);
            // velocity
            if (0 <= rel_x && rel_x < WIDTH && 0 <= rel_y && rel_y < HEIGHT) {
                const auto vec = simulator.velocity(rel_y, rel_x);
                ImGui::Text("(vx, vy) = (%.3f, %.3f)", vec.x(), vec.y());
            } else {
                ImGui::Text("(vx, vy) = ");
            }
            // pressure
            if (0 <= rel_x && rel_x < WIDTH && 0 <= rel_y && rel_y < HEIGHT) {
                const auto p = simulator.pressure(rel_y, rel_x);
                ImGui::Text("p = %.3f", p);
            } else {
                ImGui::Text("p = ");
            }

            ImGui::SliderFloat("velocity scale", &render_config.velocity_range, 1e-1f, 1e4f, "velocity scale: %.4f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("pressure scale", &render_config.pressure_range, 1e-4f, 1e0f, "pressure scale: %.4f", ImGuiSliderFlags_Logarithmic);

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
