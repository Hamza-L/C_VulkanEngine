//
// Created by hlahm on 2021-10-07.
//

#ifndef VULKANENGINE2_VE_VALIDATION_LAYER_H
#define VULKANENGINE2_VE_VALIDATION_LAYER_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
static const int enableValidationLayers = 0;
#else
static const int enableValidationLayers = 1;
#endif

static VkDebugUtilsMessengerEXT debugMessenger;
static const char* validationLayers[10] = {"VK_LAYER_KHRONOS_validation"};
static uint32_t numValidationLayer = 1;

#endif //VULKANENGINE2_VE_VALIDATION_LAYER_H
