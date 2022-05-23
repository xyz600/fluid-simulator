#pragma once

#include <cuda_gl_interop.h>
#include <cuda_runtime.h> // CUDA
#include <device_launch_parameters.h>

#include <GL/glew.h> // GL
#include <GLFW/glfw3.h>

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


class GPUFluidSimulator {   
public:
    GPUFluidSimulator(const std::size_t width, const std::size_t height, GLuint pbo)
        : width_(width)
        , height_(height)
        , pbo_(pbo)
    {
        gpuErrchk(cudaGraphicsGLRegisterBuffer(&cudapbo_, pbo_, cudaGraphicsRegisterFlagsWriteDiscard));
    }

    ~GPUFluidSimulator() {
        gpuErrchk(cudaGLUnregisterBufferObject(pbo));
        gpuErrchk(cudaGraphicsUnregisterResource(cudapbo));
    }

    void draw_background()
    {
        static unsigned char frame = 0;
        frame++;
        uchar4* dev_map;

        gpuErrchk(cudaGraphicsMapResources(1, &cudapbo, nullptr));
        gpuErrchk(cudaGraphicsResourceGetMappedPointer((void**)&dev_map, nullptr, cudapbo_));

        dim3 threads(8, 8);
        dim3 grids(width / 8, height / 8);
        kernel<<<grids, threads>>>(dev_map, frame);

        gpuErrchk(cudaDeviceSynchronize());
        gpuErrchk(cudaGraphicsUnmapResources(1, &cudapbo_, nullptr));
    }

private:
    std::size_t width_;

    std::size_t height_;

    GLuint pbo_;

    cudaGraphicsResource* cudapbo_;
};