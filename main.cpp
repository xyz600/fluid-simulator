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

#include <iostream>
#include <stdlib.h>

#include "cpu_fluid_simulator.hpp"

struct ViewerState {
    bool show_config;
    ImVec4 clear_color;

    int shape;

    ViewerState()
        : show_config(true)
        , clear_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f))
        , shape(0)
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

    constexpr std::size_t WIDTH = 1920;
    constexpr std::size_t HEIGHT = 1080;

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

    auto simulator = CPUFluidSimulator(WIDTH, HEIGHT, pbo, 1.0, 0.12);

    while (!glfwWindowShouldClose(window)) {

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        simulator.draw_background();

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
            ImGui::RadioButton("square", &state.shape, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Circle", &state.shape, 1);
            ImGui::SameLine();
            ImGui::RadioButton("None", &state.shape, 2);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); // exit
    glDeleteBuffers(1, &pbo);
    glfwTerminate();
    return 0;
}
