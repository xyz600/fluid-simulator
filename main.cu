// based on https://gist.github.com/kamino410/09df4ecdf37b03cbd05752a7b2e52d3a
// this adds ImGui to an application with CUDA and OpenGL. the thing is, once you use CUDA, ImGui renders very strangely.
// after 9 hours of debugging I found that putting glBindBuffer before and after the draw call fixes this!
// glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo); // THE MAGIC LINE #1
// glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
// glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);   // THE MAGIC LINE #2

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h> // CUDA
#include <device_launch_parameters.h>
#include <iostream>
#include <stdlib.h>

#define WIDTH 512 * 2
#define HEIGHT 512

GLuint pbo;
cudaGraphicsResource* cudapbo;

#define gpuErrchk(ans)                        \
    {                                         \
        gpuAssert((ans), __FILE__, __LINE__); \
    }
inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort = true)
{
    if (code != cudaSuccess) {
        fprintf(stderr, "GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort)
            exit(code);
    }
}

__global__ void kernel(uchar4* map, unsigned char frame)
{
    int x = threadIdx.x + blockIdx.x * blockDim.x;
    int y = threadIdx.y + blockIdx.y * blockDim.y;
    int id = x + y * blockDim.x * gridDim.x;

    map[id].x = x / 2;
    map[id].y = y / 2;
    map[id].z = frame;
    map[id].w = 255;
}

extern "C" void kernelUpdate(int width, int height)
{
    static unsigned char frame = 0;
    frame++;
    uchar4* dev_map;

    gpuErrchk(cudaGraphicsMapResources(1, &cudapbo, NULL));
    gpuErrchk(cudaGraphicsResourceGetMappedPointer((void**)&dev_map, NULL, cudapbo));

    dim3 threads(8, 8);
    dim3 grids(width / 8, height / 8);
    kernel<<<grids, threads>>>(dev_map, frame);

    gpuErrchk(cudaDeviceSynchronize());
    gpuErrchk(cudaGraphicsUnmapResources(1, &cudapbo, NULL));
}

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
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "gl-cuda-test", NULL, NULL);
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
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // CUDA with GL interop
    glGenBuffers(1, &pbo); // make & register PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 4 * sizeof(GLubyte) * WIDTH * HEIGHT, NULL, GL_DYNAMIC_DRAW);
    gpuErrchk(cudaGraphicsGLRegisterBuffer(&cudapbo, pbo, cudaGraphicsRegisterFlagsWriteDiscard));

    while (!glfwWindowShouldClose(window)) {

        kernelUpdate(WIDTH, HEIGHT);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo); // THE MAGIC LINE #1
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); // THE MAGIC LINE #2

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.
            ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        {
            ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0); // exit
    gpuErrchk(cudaGLUnregisterBufferObject(pbo));
    gpuErrchk(cudaGraphicsUnregisterResource(cudapbo));
    glDeleteBuffers(1, &pbo);
    glfwTerminate();
    return 0;
}
