
#define GLFW_INCLUDE_VULKAN
#define CGLM_DEFINE_PRINTS
#include <GLFW/glfw3.h>

#include <stdio.h>

#include "renderer/ve_renderer.c"

int main() {

    initEngine();
    renderer.run();

    return 0;
}