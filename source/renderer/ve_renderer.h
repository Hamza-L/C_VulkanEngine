//
// Created by hlahm on 2021-10-21.
//

#ifndef VULKANENGINE2_VE_RENDERER_H
#define VULKANENGINE2_VE_RENDERER_H

#include <stdio.h>

#include "../vulkan-backend/ve_backend.c"


struct Ve_renderer{
    void (*run)();
};

struct MeshObject{
    char* name;

    struct Vertex* vertices;
    uint32_t vertexCount;

    uint32_t* indices;
    uint32_t indicesCount;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

#endif //VULKANENGINE2_VE_RENDERER_H
