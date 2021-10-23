cmake_minimum_required(VERSION 3.20)
project(VulkanEngine2 C)

set(CMAKE_C_STANDARD 11)

include_directories(C:/GLFW/include)
include_directories(C:/VulkanSDK/1.2.189.2/Include)
include_directories(C:/CGLM/include)
include_directories(${PROJECT_SOURCE_DIR}/ui)

add_executable(VulkanEngine2 source/main.c source/window/ve_window.c source/window/ve_window.h source/vulkan-backend/ve_backend.c source/vulkan-backend/ve_backend.h source/vulkan-backend/ve_validation_layer.c source/vulkan-backend/ve_validation_layer.h source/utilities/utilities.h source/renderer/ve_renderer.c source/renderer/ve_renderer.h)

target_link_libraries(VulkanEngine2 "C:/GLFW/lib-mingw-w64/libglfw3.a")
target_link_libraries(VulkanEngine2 "C:/VulkanSDK/1.2.189.2/Lib/vulkan-1.lib")
target_link_libraries(VulkanEngine2 imm32.lib)
#target_link_libraries(VulkanEngine2 "${PROJECT_SOURCE_DIR}/ui")