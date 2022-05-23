cmake_minimum_required(VERSION 3.23)

# reference: 
# https://jacobaustin123.github.io/cmake/

project(fluid-simulator CUDA)

find_package(glfw3 CONFIG REQUIRED)
find_package(glew CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(fluid-simulator main.cu)

set_target_properties(fluid-simulator PROPERTIES CUDA_STANDARD 17)
set_target_properties(fluid-simulator PROPERTIES CUDA_COMPILER_ID NVIDIA)
set_target_properties(fluid-simulator PROPERTIES LANGUAGE_COMPILER "nvcc")
set_target_properties(fluid-simulator PROPERTIES CUDA_ARCHITECTURES "61")
set_target_properties(fluid-simulator PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
set_target_properties(fluid-simulator PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)

set_target_properties(fluid-simulator PROPERTIES BUILD_SHARED_LIBS ON)
set_target_properties(fluid-simulator PROPERTIES CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(fluid-simulator PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(fluid-simulator PRIVATE 
    imgui::imgui
    glfw
    GLEW::GLEW
)
