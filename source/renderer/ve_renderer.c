//
// Created by hlahm on 2021-10-21.
//

#include "ve_renderer.h"

static struct Ve_renderer renderer;
static struct MeshObject object1;

static void addVertices(struct MeshObject* meshObj, struct Vertex* vertices, int vertexCount)
{
    meshObj->vertices = realloc(meshObj->vertices, (meshObj->vertexCount + vertexCount) * sizeof(struct Vertex));

    for(int i=0; i<vertexCount; i++) {
        meshObj->vertices[meshObj->vertexCount] = vertices[i];
        meshObj->vertexCount = meshObj->vertexCount + 1;
    }
}

static void addIndices(struct MeshObject* meshObj, uint32_t* indices, uint32_t indicesCount)
{
    meshObj->indices = realloc(meshObj->indices, (meshObj->indicesCount + indicesCount) * sizeof(uint32_t));

    for(int i=0; i < indicesCount; i++) {
        meshObj->indices[meshObj->indicesCount] = indices[i];
        meshObj->indicesCount = meshObj->indicesCount + 1;
    }
}

static void initMesh(struct MeshObject* meshObj)
{

    struct Vertex vertices[] = {
            {{0.4f,0.4f,0.0f} ,   {1.0f,0.0f,0.0f}},
            {{0.4f,-0.4f,0.0f}  , {0.0f,1.0f,0.0f}},
            {{-0.4f,-0.4f,0.0f} , {0.0f,0.0f,1.0f}},
            {{-0.4f,0.4f,0.0f} ,  {1.0f,1.0f,0.0f}}
    };

    uint32_t indices[] = {  0,1,2,
                            2,3,0   };

    addVertices(meshObj, vertices, 4);
    addIndices(meshObj, indices, 6);
}

static uint32_t findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties){
    //Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties = {};
    vkGetPhysicalDeviceMemoryProperties(vulkanBE.mainDevices.physicalDevice, &memoryProperties);

    for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if((allowedTypes & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i; //this memorytype is valid so we return it.
        }
    }

    return 0;
}

static void createBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    //a buffer describes the memory. it does not include assigning memory
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = bufferUsageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(vulkanBE.mainDevices.device, &bufferCreateInfo, NULL, buffer);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr,"failed to create buffer\n");
        exit(1);
    }

    //Get Buffer memory requirements
    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(vulkanBE.mainDevices.device, *buffer, &memoryRequirements);

    //Allocate memory to buffer
    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, //index of memory type on physical device for required bit flags
                                                             bufferProperties); //the properties we want for the buffer

    result = vkAllocateMemory(vulkanBE.mainDevices.device, &memoryAllocateInfo, NULL, bufferMemory);
    if ( result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to allocate vertex buffer memory");
        exit(1);
    }

    //allocate memory to given vertex buffer
    vkBindBufferMemory(vulkanBE.mainDevices.device, *buffer, *bufferMemory, 0);

}

static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
    //commandBuffer to hold transfer commands
    VkCommandBuffer transferCommandBuffer;

    //command buffer details
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vulkanBE.pools.transferCommandPool;
    allocInfo.commandBufferCount = 1;

    //Allocate command buffer from pool;
    vkAllocateCommandBuffers(vulkanBE.mainDevices.device, &allocInfo, &transferCommandBuffer);

    //information to begin the commandbuffer
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; //we're only using the command buffer once.

    //begin recording command for transfer
    vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    //copy region for vkCmdCopyBuffer
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0; //we want to copy from the start and to the start
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion); //command to copy out a src buffer to dst buffer

    vkEndCommandBuffer(transferCommandBuffer); //end command

    //queue submission
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    vkQueueSubmit(vulkanBE.transferQueue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(vulkanBE.transferQueue);

    vkFreeCommandBuffers(vulkanBE.mainDevices.device, vulkanBE.pools.transferCommandPool, 1, &transferCommandBuffer);
}

static void createVertexBuffer(struct MeshObject* meshObj)
{
    VkDeviceSize bufferSize = sizeof(struct Vertex) * meshObj->vertexCount;

    //temporary buffer to stage vertex data before transfering to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    //map memory to vertexbuffer
    void* data;                                                                                           //1. create a pointer a point in normal memory;
    vkMapMemory(vulkanBE.mainDevices.device, stagingBufferMemory, 0, bufferSize, 0 , &data);   //2. map the vertex buffer memory to that point.
    memcpy(data, meshObj->vertices, (size_t)bufferSize);                                                  //3. copies memory from vertices vector to the point in memory
    vkUnmapMemory(vulkanBE.mainDevices.device, stagingBufferMemory);                                      //4. unmap the vertex memory

    //create buffer to TRANSFER_DST_BIT to mark as recipient for transfer data;
    //buffer memory is to be device local bit meaning memory is on the GPU and only accessible by it.
    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &meshObj->vertexBuffer, &meshObj->vertexBufferMemory);

    //copy staging buffer to gpu memory buffer
    copyBuffer(stagingBuffer, meshObj->vertexBuffer, bufferSize);

    //clean up staging buffer
    vkDestroyBuffer(vulkanBE.mainDevices.device, stagingBuffer, NULL);
    vkFreeMemory(vulkanBE.mainDevices.device, stagingBufferMemory, NULL);
}

static void createIndexBuffer(struct MeshObject* meshObj)
{
    //get size of buffer needed ofr indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * meshObj->indicesCount;

    //temporary buffer to stage index data before transfering to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    //map memory to indexbuffer
    void* data;                                                                                           //1. create a pointer a point in normal memory;
    vkMapMemory(vulkanBE.mainDevices.device, stagingBufferMemory, 0, bufferSize, 0 , &data);   //2. map the index buffer memory to that point.
    memcpy(data, meshObj->indices, (size_t)bufferSize);                                                   //3. copies memory from indices vector to the point in memory
    vkUnmapMemory(vulkanBE.mainDevices.device, stagingBufferMemory);                                      //4. unmap the index memory

    //Create buffer for index data on GPU access only area
    createBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &meshObj->indexBuffer, &meshObj->indexBufferMemory);

    //Copy from staging buffer to GPU access buffer
    copyBuffer(stagingBuffer, meshObj->indexBuffer, bufferSize);

    //Destroy + release staging buffer resources
    vkDestroyBuffer(vulkanBE.mainDevices.device, stagingBuffer, NULL);
    vkFreeMemory(vulkanBE.mainDevices.device, stagingBufferMemory, NULL);

}

static void destroyVertexBuffer(struct MeshObject* meshObj)
{
    vkDestroyBuffer(vulkanBE.mainDevices.device, meshObj->vertexBuffer, NULL);
    vkFreeMemory(vulkanBE.mainDevices.device, meshObj->vertexBufferMemory, NULL);
}

static void destroyIndexBuffer(struct MeshObject* meshObj)
{
    vkDestroyBuffer(vulkanBE.mainDevices.device, meshObj->indexBuffer, NULL);
    vkFreeMemory(vulkanBE.mainDevices.device, meshObj->indexBufferMemory, NULL);
}

static void cleanUpRenderer()
{
    vkDeviceWaitIdle(vulkanBE.mainDevices.device);
    destroyVertexBuffer(&object1);
    destroyIndexBuffer(&object1);
}

static void recordCommand(){

    //information about how to begin each command buffers
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; //buffer can be resubmitted when it has already been submitted and is awaiting execution.

    //information about how to begin a renderpass (only needed for graphical application)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = vulkanBE.renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = vulkanBE.imageInfo.extent; //size of region to run renderpass on.

    //assigns to each attachment. clears attachment[1] to cleavalues[1]
    VkClearValue clearValues[] = { //add depth attachment lear value
            {0.3f,0.3f,0.3f,1.0f}
    };
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.clearValueCount = 1;


    for(size_t i = 0; i < vulkanBE.num_swapchain_images; i++)
    {
        //select the framebuffer to render to
        renderPassBeginInfo.framebuffer = vulkanBE.swapChainFrameBuffers[i];

        //start recording command with command buffer
        VkResult result = vkBeginCommandBuffer(vulkanBE.commandBuffers[i],&bufferBeginInfo);
        if(result != VK_SUCCESS){
            fprintf(stderr, "failed to start recording a command buffer\n");
            exit(1);
        }

        vkCmdBeginRenderPass(vulkanBE.commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); // all the commands will be primary

            //bind pipeline to be used in renderpass
            vkCmdBindPipeline(vulkanBE.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanBE.graphicsPipeline);

            VkBuffer vertexBuffers[] = {object1.vertexBuffer};      //buffers to bind
            VkDeviceSize offsets[] = {0};                           //offset into buffer being bound
            vkCmdBindVertexBuffers(vulkanBE.commandBuffers[i], 0, 1, vertexBuffers, offsets);

            //binding the index buffer with 0 offset using the uint32type
            vkCmdBindIndexBuffer(vulkanBE.commandBuffers[i], object1.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            //number of times to draw/run the shader/pipeline.
        vkCmdDrawIndexed(vulkanBE.commandBuffers[i], object1.indicesCount, 1, 0, 0, 0);

        vkCmdEndRenderPass(vulkanBE.commandBuffers[i]); // all the commands will be primary


        result = vkEndCommandBuffer(vulkanBE.commandBuffers[i]);
        if(result != VK_SUCCESS){
            fprintf(stderr, "failed to stop recording a command buffer\n");
            exit(1);
        }
    }

}

static void draw()
{
    //1-get the next available image to draw to and set something to signal when we're finished with the image (a semaphore)
    //2-submit command buffer to queue for execution. making sure it waits for the image to be signalled as available before drawing and signals when it has finished rendering.
    //3-present image to screen when it has signaled finished rendering.

    vkWaitForFences(vulkanBE.mainDevices.device, 1, &vulkanBE.sync.drawFences[currentFrame], VK_TRUE, INT_MAX); //freeze the code until the fence is signaled
    vkResetFences(vulkanBE.mainDevices.device, 1, &vulkanBE.sync.drawFences[currentFrame]); // manually unsignal the fence, so we can carry on with the code.

    //Get Next Image index and signal semaphore when reading to be drawn to:
    uint32_t imageIndex;
    vkAcquireNextImageKHR(vulkanBE.mainDevices.device, vulkanBE.swapChain, INT_MAX, vulkanBE.sync.imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

    //submit command buffer to render
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &vulkanBE.sync.imageAvailable[currentFrame];
    VkPipelineStageFlags waitStages[] = { //as soon as this stage of the pipeline is reached, the semaphore will be waited for.
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1; //number of command buffers to submit
    submitInfo.pCommandBuffers = &vulkanBE.commandBuffers[imageIndex]; //command buffer to submit
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &vulkanBE.sync.renderFinished[currentFrame]; //semaphore to signal when command buffer is finished submitting

    //submit command buffer to queue
    VkResult result = vkQueueSubmit(vulkanBE.graphicsQueue, 1, &submitInfo, vulkanBE.sync.drawFences[currentFrame]);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to submit the command buffer to the graphics queue\n");
        exit(1);
    }

    // present rendered image to the screen
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &vulkanBE.sync.renderFinished[currentFrame]; //semaphore to wait on
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkanBE.swapChain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vulkanBE.presentQueue, &presentInfo);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to present image\n");
        exit(1);
    }
    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

static void run(){
    recordCommand();
    while(!glfwWindowShouldClose(ve_window.window)){
        glfwPollEvents();
        draw();
    }

    glfwDestroyWindow(ve_window.window);
    cleanUpRenderer();
    cleanUpBackend();

    glfwTerminate();
}

static void initEngine(){
    renderer.run = run;
    initBackend();
    initMesh(&object1);
    createVertexBuffer(&object1);
    createIndexBuffer(&object1);
}