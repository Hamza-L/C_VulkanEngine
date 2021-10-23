//
// Created by hlahm on 2021-10-02.
//

#ifndef VULKANENGINE2_VE_WINDOW_H
#define VULKANENGINE2_VE_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>

struct Ve_window{
    GLFWwindow * window;
    uint32_t WIDTH;
    uint32_t HEIGHT;
};


#endif //VULKANENGINE2_VE_WINDOW_H
