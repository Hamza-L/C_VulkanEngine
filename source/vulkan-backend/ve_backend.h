//
// Created by hlahm on 2021-10-04.
//

#ifndef VULKANENGINE2_VE_BACKEND_H
#define VULKANENGINE2_VE_BACKEND_H

#include <stdexcpt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>
#include <cglm/mat4.h>
#include <cglm/io.h>

#include "../window/ve_window.c"
#include "ve_validation_layer.c"
#include "../utilities/utilities.h"

enum { MAX_INSTANCE_EXTENSIONS = 64, MAX_DEVICE_EXTENSIONS = 64, MAX_SURFACE_FORMATS = 64, MAX_PRESENTATION_MODE = 12 , MAX_IMAGE = 3, MAX_FRAME_DRAWS = 2};

static int currentFrame = 0;

struct devices{
    VkPhysicalDevice physicalDevice;
    VkDevice device;
};

struct QueueFamilyIndices{
    int graphicsFamily;
    int presentationFamily;
};

struct SwapChainDetails{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;               //surface properties (image size etc...)
    VkSurfaceFormatKHR* surfaceFormat;      //surface formats (RGBA etc...)
    uint32_t surfaceFormat_size;
    VkPresentModeKHR* presentationModes; //surface formats
    uint32_t presentationModes_size;
};

struct ImageInfo{
    VkSurfaceFormatKHR surfaceFormat;
    VkExtent2D extent;
};

struct SwapChainImage{
    VkImage image;
    VkImageView imageView;
};

struct Pools{
    VkCommandPool graphicsCommandPool;
    VkCommandPool transferCommandPool;
};

struct Sync{
    VkSemaphore* imageAvailable;
    VkSemaphore* renderFinished;
    VkFence* drawFences;
};

struct Ve_backend{
    VkInstance instance;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkRenderPass renderPass;
    struct devices mainDevices;
    struct QueueFamilyIndices queueFamilyIndices;
    struct SwapChainDetails swapChainDetails;
    struct Sync sync;

    //these are all the same size;
    //commandBuffer[1] will only ever output to swapChainFrameBuffer[1], which will only ever use swapChainImage[1]
    struct SwapChainImage* swapChainImages;
    VkFramebuffer* swapChainFrameBuffers;
    VkCommandBuffer* commandBuffers;

    struct ImageInfo imageInfo;
    struct Pools pools;
    uint32_t num_swapchain_images;

    void (*draw)();
};

#endif //VULKANENGINE2_VE_BACKEND_H
