//
// Created by hlahm on 2021-10-04.
//

#include "ve_backend.h"

static struct Ve_backend vulkanBE;

static const char* instance_extentions[MAX_INSTANCE_EXTENSIONS];
static uint32_t num_instance_extensions;
static const char* deviceExtensions[MAX_DEVICE_EXTENSIONS] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static uint32_t num_device_extensions = 1;
static VkPhysicalDeviceFeatures device_features = {};

static void initVulkanBE(){
    vulkanBE.queueFamilyIndices.graphicsFamily = -1;
    vulkanBE.queueFamilyIndices.presentationFamily = -1;
    vulkanBE.swapChainDetails.surfaceFormat = calloc(MAX_SURFACE_FORMATS,sizeof(VkSurfaceFormatKHR));
    vulkanBE.swapChainDetails.presentationModes = calloc(MAX_PRESENTATION_MODE,sizeof(VkPresentModeKHR));
    vulkanBE.swapChainImages = calloc(MAX_IMAGE, sizeof(struct ImageInfo));
}

static void cleanUpVulkanBE(){
    free(vulkanBE.swapChainDetails.surfaceFormat);
    free(vulkanBE.swapChainDetails.presentationModes);
}

static void getQueueFamilies(VkPhysicalDevice physicalDevice)
{

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamilyList[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyList);

    //Go through each queue family and check if it has at least 1 of the required types of queue
    for(int i = 0; i < queueFamilyCount; i++){
        if(queueFamilyList[i].queueCount > 0 && queueFamilyList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            vulkanBE.queueFamilyIndices.graphicsFamily = i; //if queue family is appropriate, save it.
        }

        VkBool32 presentationSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vulkanBE.surface, &presentationSupport);

        if(queueFamilyList[i].queueCount > 0 && presentationSupport){
            vulkanBE.queueFamilyIndices.presentationFamily = i;
        }

        if(vulkanBE.queueFamilyIndices.graphicsFamily >= 0 && vulkanBE.queueFamilyIndices.presentationFamily >= 0){
            break;
        }
    }

}

static int checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const char ** checkExtensions, uint32_t numExtentions){
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL);

    //create a list of VkExtensionProperties using count;
    VkExtensionProperties extensions[extensionCount];
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions);

    //check if extentions are in list of available extentions
    for(int i = 0; i < numExtentions; i++){
        int hasExtensions = 0;
        for(int j = 0; j < extensionCount; j++){
            if(strcmp(checkExtensions[i],extensions[j].extensionName) == 0){
                hasExtensions = 1;
                break;
            }
        }

        if (!hasExtensions){
            return 0;
        }
    }

    return 1;
}

static void getSwapChainDetails(VkPhysicalDevice physicalDevice)
{
    //get surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vulkanBE.surface, &vulkanBE.swapChainDetails.surfaceCapabilities);

    //get formats
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanBE.surface, &formatCount, NULL);
    if (formatCount != 0){
        vulkanBE.swapChainDetails.surfaceFormat = realloc(vulkanBE.swapChainDetails.surfaceFormat ,formatCount * sizeof(VkSurfaceFormatKHR));
        vulkanBE.swapChainDetails.surfaceFormat_size = formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanBE.surface, &formatCount , vulkanBE.swapChainDetails.surfaceFormat);
    }

    //get presentationmode
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vulkanBE.surface, &presentationCount, NULL);
    if (presentationCount != 0){
        vulkanBE.swapChainDetails.presentationModes = realloc(vulkanBE.swapChainDetails.presentationModes, (presentationCount) * sizeof(VkPresentModeKHR));
        vulkanBE.swapChainDetails.presentationModes_size = presentationCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, vulkanBE.surface, &presentationCount , vulkanBE.swapChainDetails.presentationModes);
    }
}

static int checkIfDeviceSuitable(VkPhysicalDevice physicalDevice)
{
    // information about the device itself
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    // information about what device can do.
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    getQueueFamilies(physicalDevice);


    int queueSupported = vulkanBE.queueFamilyIndices.graphicsFamily >= 0 && vulkanBE.queueFamilyIndices.presentationFamily >= 0;
    int extensionSupported = checkDeviceExtensionSupport(physicalDevice, deviceExtensions, num_device_extensions);

    if(!extensionSupported){
        return 0;
    }

    getSwapChainDetails(physicalDevice);

    int swapchainSupported = vulkanBE.swapChainDetails.surfaceFormat != NULL && vulkanBE.swapChainDetails.presentationModes != NULL;

    return queueSupported && swapchainSupported;
}

static void getPhysicalDevice()
{
    //enumerate physical device the VkInstance can access.
    uint32_t numDevice = 0;
    vkEnumeratePhysicalDevices(vulkanBE.instance, &numDevice, NULL);

    if (numDevice == 0){
        printf("can't find GPU that support vulkan instance!");
    }

    //get list of physical Devices
    VkPhysicalDevice deviceList[numDevice];
    vkEnumeratePhysicalDevices(vulkanBE.instance, &numDevice, deviceList);

    VkPhysicalDeviceProperties deviceProperties;

    vkGetPhysicalDeviceProperties(deviceList[0], &deviceProperties);

    for(int i = 0; i < numDevice; i++){
        if(checkIfDeviceSuitable(deviceList[i])){
            printf("Physical Device Used: %s\n", deviceProperties.deviceName);
            vulkanBE.mainDevices.physicalDevice = deviceList[0];
        }
    }

}

static int checkInstanceExtensionSupport(const char ** checkExtensions, uint32_t numExtentions)
{
    uint32_t extentionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extentionCount, NULL);

    //create a list of VkExtensionProperties using count;
    VkExtensionProperties extensions[extentionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &extentionCount, extensions);

    //check if extentions are in list of available extentions
    for(int i = 0; i < numExtentions; i++){
        int hasExtensions = 0;
        for(int j = 0; j < extentionCount; j++){
            if(strcmp(checkExtensions[i],extensions[j].extensionName) == 0){
                hasExtensions = 1;
                break;
            }
        }

        if (!hasExtensions){
            return 0;
        }
    }

    return 1;
}

static void createInstance()
{

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        printf("validation layers requested, but not available!");
    }

    //information about the application itself. This does not affect the program and is only for dev convenience.
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VE_App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "VulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t glfwExtensionCount = 0; //GLFW may require many extension
    const char** glfwExtensions; //Extensions passed as array of array of char.

    //get GLFW Extensions
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for(size_t i = 0; i < glfwExtensionCount; i++){
        instance_extentions[i] = glfwExtensions[i];
        num_instance_extensions++;
    }

    if(enableValidationLayers){
        instance_extentions[num_instance_extensions++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    //check if the instance extentions are supported
    if(!checkInstanceExtensionSupport(instance_extentions, num_instance_extensions)){
        printf("not all extensions are supported!");
    }

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = num_instance_extensions;
    createInfo.ppEnabledExtensionNames = instance_extentions;
    createInfo.ppEnabledLayerNames = NULL;

    //enable validation layer
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = numValidationLayer;
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = NULL;
    }

    //create instance
    VkResult result = vkCreateInstance(&createInfo,NULL,&vulkanBE.instance);
    if(result != VK_SUCCESS){
        printf("Instance creation was unsuccessful");
    }
}

static void createLogicalDevice()
{

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.enabledExtensionCount = num_device_extensions; //number of LOGICAL device extensions
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &device_features;

    //Queue the logical device needs to create and info to do so.
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vulkanBE.queueFamilyIndices.graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority; //if we have multiple queues running at the same time, which one has priority. 1 is highest and low is lowers.

    if(vulkanBE.queueFamilyIndices.graphicsFamily != vulkanBE.queueFamilyIndices.presentationFamily)
    {
        VkDeviceQueueCreateInfo queueCreateInfo2 = {};
        queueCreateInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo2.queueFamilyIndex = vulkanBE.queueFamilyIndices.presentationFamily;
        queueCreateInfo2.queueCount = 1;
        queueCreateInfo2.pQueuePriorities = &queuePriority;

        VkDeviceQueueCreateInfo queuesInfos[2] = {queueCreateInfo,queueCreateInfo2};

        //information to create logical device


        deviceCreateInfo.queueCreateInfoCount = (uint32_t)2;
        deviceCreateInfo.pQueueCreateInfos = queuesInfos;

        VkResult result = vkCreateDevice(vulkanBE.mainDevices.physicalDevice, &deviceCreateInfo, NULL, &vulkanBE.mainDevices.device);
        if (result != VK_SUCCESS){
            printf("Failed to create logical device");
        }

        //Queue are created at the same time as device. now we want to get queue handles.
        vkGetDeviceQueue(vulkanBE.mainDevices.device, vulkanBE.queueFamilyIndices.graphicsFamily, 0, &vulkanBE.graphicsQueue);
        vkGetDeviceQueue(vulkanBE.mainDevices.device, vulkanBE.queueFamilyIndices.presentationFamily, 0, &vulkanBE.presentQueue);

    } else
    {
        //information to create logical device
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;


        VkResult result = vkCreateDevice(vulkanBE.mainDevices.physicalDevice, &deviceCreateInfo, NULL, &vulkanBE.mainDevices.device);
        if (result != VK_SUCCESS){
            printf("Failed to create logical device");
        }

        //Queue are created at the same time as device. now we want to get queue handles.
        vkGetDeviceQueue(vulkanBE.mainDevices.device, vulkanBE.queueFamilyIndices.graphicsFamily, 0, &vulkanBE.graphicsQueue);
        vkGetDeviceQueue(vulkanBE.mainDevices.device, vulkanBE.queueFamilyIndices.graphicsFamily, 0, &vulkanBE.presentQueue);
    }

    //as per vulkan standard, transferqueue is always graphicsqueue
    vulkanBE.transferQueue = vulkanBE.graphicsQueue;

}

static void createSurface()
{
    //creates a surface create info struct, runs the surface create function and returns a VK_Result. It is essentially a wrapper function
    VkResult result = glfwCreateWindowSurface(vulkanBE.instance, ve_window.window, NULL, &vulkanBE.surface);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Surface!\n");
        exit(1);
    }

}

//format we will choose is          VK_FORMAT_R8G8B8A8_UNORM
//colourspace we will choose is     VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
static VkSurfaceFormatKHR chooseSwapchainFormat()
{
    VkSurfaceFormatKHR bestFormat = {};
    if(vulkanBE.swapChainDetails.surfaceFormat_size == 1 && vulkanBE.swapChainDetails.surfaceFormat[0].format == VK_FORMAT_UNDEFINED )
    {
        bestFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
        bestFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        return bestFormat;
    }

    for(int i = 0; i < vulkanBE.swapChainDetails.surfaceFormat_size; i++){
        if(vulkanBE.swapChainDetails.surfaceFormat[i].format == VK_FORMAT_R8G8B8A8_UNORM || vulkanBE.swapChainDetails.surfaceFormat[i].format == VK_FORMAT_B8G8R8A8_UNORM && vulkanBE.swapChainDetails.surfaceFormat[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return vulkanBE.swapChainDetails.surfaceFormat[i];
        }
    }

    return vulkanBE.swapChainDetails.surfaceFormat[0];
}

static VkPresentModeKHR chooseSwapchainPresentMode()
{

    //look for mailbox Presentation mode. if not found, use FIFO
    for(int i = 0; i < vulkanBE.swapChainDetails.presentationModes_size; i++){
        if(vulkanBE.swapChainDetails.presentationModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return vulkanBE.swapChainDetails.presentationModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D chooseSwapChainExtent()
{
    if(vulkanBE.swapChainDetails.surfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        //if value is fixed, we return it.
        return vulkanBE.swapChainDetails.surfaceCapabilities.currentExtent;
    }
    else
    {   //if the value can vary, we need to set it manually.
        int width,height;
        glfwGetFramebufferSize(ve_window.window, &width, &height);

        VkExtent2D newExtent = {};
        newExtent.width = (uint32_t) width;
        newExtent.height = (uint32_t) height;

        //surface also defines max and min so we need to make sure we are within boundaries
        if(newExtent.width > vulkanBE.swapChainDetails.surfaceCapabilities.maxImageExtent.width || newExtent.width < vulkanBE.swapChainDetails.surfaceCapabilities.minImageExtent.width)
        {
            newExtent.width = vulkanBE.swapChainDetails.surfaceCapabilities.maxImageExtent.width;
        }

        if(newExtent.height > vulkanBE.swapChainDetails.surfaceCapabilities.maxImageExtent.height || newExtent.height < vulkanBE.swapChainDetails.surfaceCapabilities.minImageExtent.height)
        {
            newExtent.height = vulkanBE.swapChainDetails.surfaceCapabilities.maxImageExtent.height;
        }

        return newExtent;
    }
}

static VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlagBits aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.pNext = NULL;
    viewCreateInfo.flags = 0;
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;   //image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  //image type (2D, 3D etc)
    viewCreateInfo.format = format;     //format of image fata
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;  //allows remapping of colour values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    //Sub ressources allow the view to view only part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags; //which aspect of image to view (COLOUR BIT for our case)
    viewCreateInfo.subresourceRange.baseMipLevel = 0; //start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1; //number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0; //start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1; //number of array levels to view

    //create image view and return it;
    VkImageView imageView;
    VkResult result = vkCreateImageView(vulkanBE.mainDevices.device, &viewCreateInfo, NULL, &imageView);
    if(result != VK_SUCCESS){
        fprintf(stderr, "Failed to create an image view\n");
        exit(1);
    }

    return imageView;
}

static void createSwapChain()
{
    //choose best surface format
    VkSurfaceFormatKHR surfaceFormat = chooseSwapchainFormat();

    //choose best presentation mode
    VkPresentModeKHR presentMode = chooseSwapchainPresentMode();

    //chose swapchain resolution
    VkExtent2D extent = chooseSwapChainExtent();

    //how many images are in the swapChain. we will get one more than the min to allow for triple buffer.
    uint32_t maxImageCount = vulkanBE.swapChainDetails.surfaceCapabilities.minImageCount; // if 0, it is limitless.

    if(maxImageCount > vulkanBE.swapChainDetails.surfaceCapabilities.maxImageCount && maxImageCount > 0){
        maxImageCount = vulkanBE.swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.minImageCount = maxImageCount;
    swapChainCreateInfo.imageArrayLayers = 1; //number of layers for each image in the swapchain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //what attachment images will be used. you shouldn't but depth data in swapchain.
    swapChainCreateInfo.preTransform = vulkanBE.swapChainDetails.surfaceCapabilities.currentTransform; //transform to perform on swapchain images.
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //don't do any blending of the window with external graphics (eg other windows)
    swapChainCreateInfo.clipped = VK_TRUE; //how to clip with external graphics. if a window overlaps with our window, do not render and clip.

    //if graphics and presentation families are different then swapchain must let images be shared between families;
    if(vulkanBE.queueFamilyIndices.graphicsFamily != vulkanBE.queueFamilyIndices.presentationFamily)
    {
        uint32_t queueFamilyIndices[2] = {(uint32_t)vulkanBE.queueFamilyIndices.graphicsFamily, (uint32_t)vulkanBE.queueFamilyIndices.presentationFamily};

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = NULL;
    }

    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE; //in case we destroy a swapchain and want to recreate it. we can use the old swapchain to quickly hand over responsibilities.
    swapChainCreateInfo.surface = vulkanBE.surface;

    VkResult result = vkCreateSwapchainKHR(vulkanBE.mainDevices.device, &swapChainCreateInfo, NULL, &vulkanBE.swapChain);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Swapchain\n");
        exit(1);
    }

    vulkanBE.imageInfo.surfaceFormat = surfaceFormat;
    vulkanBE.imageInfo.extent = extent;

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(vulkanBE.mainDevices.device, vulkanBE.swapChain, &swapChainImageCount, NULL);
    VkImage* images = calloc(swapChainImageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(vulkanBE.mainDevices.device, vulkanBE.swapChain, &swapChainImageCount, images);

    vulkanBE.swapChainImages = realloc(vulkanBE.swapChainImages, swapChainImageCount * sizeof(struct SwapChainImage));
    vulkanBE.num_swapchain_images = swapChainImageCount;

    for(int i = 0; i<swapChainImageCount; i++)
    {
        //create image
        vulkanBE.swapChainImages[i].image = images[i];

        //create image view here
        vulkanBE.swapChainImages[i].imageView = createImageView(images[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

static VkShaderModule createShaderModule(struct ShaderCode* shaderCode)
{
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shaderCode->code_size;
    shaderModuleCreateInfo.pCode = (const uint32_t *)shaderCode->code;
    shaderModuleCreateInfo.pNext = NULL;
    shaderModuleCreateInfo.flags = 0;

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(vulkanBE.mainDevices.device, &shaderModuleCreateInfo, NULL, &shaderModule);
    if(result != VK_SUCCESS){
        fprintf(stderr, "failed to create shader module\n");
        exit(1);
    }
    return shaderModule;
}

static void createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = vulkanBE.imageInfo.surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //number of sample to write for MultiSampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //what to do with attachment before rendering
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //we want to present it so we have to store it somewhere.
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //describes what to do with stencil before rendering
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //we do not care once again.

    //framebuffer data will be stored as image, but images can be given different data layouts to give optimal use for certain operations.
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //image layout before the render pass starts, before all subpass start.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  //image data layout after render pass. this is at the end of all subpass.

    //attachment references uses attachment index that refers to index in the attachment list passed to renderpasscreateinfo
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0; //use the first attachment on that list of color attachment passed unto the renderpass.
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; //this is the subpass layout.

    //Subpass info
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //we want it to bind to what type of pipeline? a graphics pipeline.
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference; //we cannot pass the color attachment directly. we have to pass a color attachment reference.

    //need to determine when layout transitions occur using subpass dependencies. these create implicit layout transitions.
    VkSubpassDependency subpassDependency[2];
    //conversion from VK_IMAGE_LAYOUT_UNDEFINED TO VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. The transition is BETWEEN src and dst
    //-----------------------------------------------conversion has to happen after we go to subpass external and attempt to read form it.
    subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL; //anything outside of our subpass
    subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; //the end of the pipeline; what has to happen first
    subpassDependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; //it has to be read before converting it to the color attachment optimal
    //-----------------------------------------------conversion has to happen before we got to the first subpass and we can read and write from it.
    subpassDependency[0].dstSubpass = 0; //the first subpass.
    subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependency[0].dependencyFlags = 0;

    //conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL TO VK_IMAGE_LAYOUT_PRESENT_SRC_KHR. The transition is BETWEEN src and dst
    //-----------------------------------------------conversion has to happen after ...
    subpassDependency[1].srcSubpass = 0;
    subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //-----------------------------------------------conversion has to happen before ...
    subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependency[1].dependencyFlags = 0;

    //Renderpass INFO
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 2;
    renderPassCreateInfo.pDependencies = subpassDependency;


    VkResult result = vkCreateRenderPass(vulkanBE.mainDevices.device, &renderPassCreateInfo, NULL, &vulkanBE.renderPass);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create a renderpass\n");
        exit(1);
    }


}

static void createGraphicsPipeline()
{

    //read in SPIR-V Code
    struct ShaderCode vertexShaderCode = readSPRVFile("C:/Users/hlahm/Documents/gitProjects/VulkanEngine2/source/shader/vert.spv");
    struct ShaderCode fragShaderCode = readSPRVFile("C:/Users/hlahm/Documents/gitProjects/VulkanEngine2/source/shader/frag.spv");

    //Build shader Modules to link to graphics pipeline
    VkShaderModule vertexShaderModule = createShaderModule(&vertexShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(&fragShaderCode);

    //vertex stage create info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main"; //shader's first function (entry point)

    //fragment stage create info
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.module = fragShaderModule;
    fragmentShaderCreateInfo.pName = "main"; //shader's first function (entry point)

    //put shadercreateinfo in array.
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    //how the data for a single vertex (position color etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                         //can bind multiple streams of data, this defines which one. this can be seen directly on the shader.
    bindingDescription.stride = sizeof(struct Vertex);      //size of single Vertex Object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //draw one vertex at a time or draw the vertex of each instances at a time (VK_VERTEX_INPUT_RATE_INSTANCES).

    //how the data for an attribute is defined within a vertex
    //position attribute
    VkVertexInputAttributeDescription positionAttributeDescription = {};
    positionAttributeDescription.binding = 0;
    positionAttributeDescription.location = 0;
    positionAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT; //what a vec3 is defined as memory wise
    positionAttributeDescription.offset = offsetof(struct Vertex, position); //the offset within the structure

    VkVertexInputAttributeDescription colorAttributeDescription = {};
    colorAttributeDescription.binding = 0;
    colorAttributeDescription.location = 1;
    colorAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT; //what a vec3 is defined as memory wise
    colorAttributeDescription.offset = offsetof(struct Vertex, color); //the offset within the structure

    VkVertexInputAttributeDescription attributeDescription[2] = {positionAttributeDescription, colorAttributeDescription};

    //Vertex input (TODO: Put in vertex description and ressource created)
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription; //list of vertex binding description (data spacing and stride information).
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2; //number of attributes.
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescription; //list of Vertex Binding Attribute Descriptions (data format and where to bind to/from)

    //input assembly (how to assemble the input into primitive shapes).
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //primitive type to assemble vertices
    inputAssembly.primitiveRestartEnable = VK_FALSE; //allow overriding of strip topology to start new primitive.

    //viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkanBE.imageInfo.extent.width;
    viewport.height = (float)vulkanBE.imageInfo.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = vulkanBE.imageInfo.extent;

    VkPipelineViewportStateCreateInfo  viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    //rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE; //we don't want object beyond the far plane to be clipped off or clamped (needs GPU feature).
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; //after doing all the pipeline stages, we discard the rasterization stage. we want to use the rasterizer so keep it false.
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE; //whether to add depth bias to fragment to stopping shadow acne

    //multisampling
    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE; //whether multisampling shading is enabled;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //takes only one sample in the middle of that fragment

    //blending
    //blend attachment state
    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_TRUE; //enable blending.

    //blending uses the following equation (srcColorBlendFactor * newColor) colorBlendOp (dstColorBlendFactor * oldColor);
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    //(1*newAlpha) + (0 * oldAlpha) = newAlpha.

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE; //alternative to calculation is to use logic operators
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    //pipeline layout (TODO: apply future descriptor set layout and push constants)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = NULL;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    // dynamic state would be created here (something alterable via command buffer)

    //pipeline layout creation
    VkResult result = vkCreatePipelineLayout(vulkanBE.mainDevices.device, &pipelineLayoutCreateInfo, NULL, &vulkanBE.pipelineLayout);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create pipeline Layout\n");
        exit(1);
    }

    //setup depth stencil (TODO:setup depth stencil testing)

    //Graphics pipeline creation
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;  //number of shader stages
    pipelineCreateInfo.pStages = shaderStages; //list of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo; //all the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = NULL;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = NULL;
    pipelineCreateInfo.layout = vulkanBE.pipelineLayout; //pipeline layout pipeline sould use
    pipelineCreateInfo.renderPass = vulkanBE.renderPass; //Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0; //subpass of renderpass to use with pipeline.

    //pipeline derivatives. can create multiple pipeline that device form one another for optimization.
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; //useful for creating a pipeline from a template
    pipelineCreateInfo.basePipelineIndex = -1; //can create multiple pipelines at once. this is the index of the pipeline which the creation of pipelines should base itself on.

    result = vkCreateGraphicsPipelines(vulkanBE.mainDevices.device,VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &vulkanBE.graphicsPipeline);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create graphics pipeline\n");
        exit(1);
    }

    //destroy shader module and code
    vkDestroyShaderModule(vulkanBE.mainDevices.device, fragShaderModule, NULL);
    vkDestroyShaderModule(vulkanBE.mainDevices.device, vertexShaderModule, NULL);
    free(vertexShaderCode.code);
    free(fragShaderCode.code);
}

static void createFrameBuffers()
{
    //resize framebuffer count to equal swapchain image count
    vulkanBE.swapChainFrameBuffers = realloc(vulkanBE.swapChainFrameBuffers, vulkanBE.num_swapchain_images * sizeof(VkFramebuffer));

    //create a framebuffer for each swapChain images
    for(size_t i=0; i<vulkanBE.num_swapchain_images; i++)
    {
        //1:1 relationship with the renderpass attachment output.
        VkImageView attachments[] = {
                vulkanBE.swapChainImages[i].imageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = vulkanBE.renderPass; //renderpass the framebuffer will be used with
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = attachments; //list of attachment 1:1 with renderpass attachment
        framebufferCreateInfo.width = vulkanBE.imageInfo.extent.width; //framebuffer width
        framebufferCreateInfo.height = vulkanBE.imageInfo.extent.height; //framebuffer height
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(vulkanBE.mainDevices.device, &framebufferCreateInfo, NULL, &vulkanBE.swapChainFrameBuffers[i]);
        if(result != VK_SUCCESS)
        {
            fprintf(stderr, "failed to create framebuffer\n");
            exit(1);
        }

    }
}

static void createCommandPool(){
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = vulkanBE.queueFamilyIndices.graphicsFamily; //queue family that buffers from this command pool will use

    VkResult result = vkCreateCommandPool(vulkanBE.mainDevices.device, &poolCreateInfo, NULL, &vulkanBE.pools.graphicsCommandPool);
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create commandpool\n");
        exit(1);
    }
    vulkanBE.pools.transferCommandPool = vulkanBE.pools.graphicsCommandPool;
}

static void createCommandBuffers()
{
    //resize commandBuffer count to equal swapchain image count
    vulkanBE.commandBuffers = realloc(vulkanBE.commandBuffers, vulkanBE.num_swapchain_images * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = vulkanBE.pools.graphicsCommandPool;
    cbAllocInfo.commandBufferCount = vulkanBE.num_swapchain_images;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; //directly submits to queue. can't be called by other buffers.
                                                            //VK_COMMAND_BUFFER_LEVEL_SECONDARY means it cannot be submitted to queue and can only be called by other commanbuffers

    //we do not create command buffers. we allocate. as soon as commandpool is destroyed, so are commandbuffers.
    VkResult result = vkAllocateCommandBuffers(vulkanBE.mainDevices.device, &cbAllocInfo, vulkanBE.commandBuffers); //allocates all the command buffers at once
    if(result != VK_SUCCESS)
    {
        fprintf(stderr, "failed to allocate command buffers\n");
        exit(1);
    }
}

static void createSynchronization()
{
    vulkanBE.sync.imageAvailable = realloc(vulkanBE.sync.imageAvailable, MAX_FRAME_DRAWS * sizeof(VkSemaphore));
    vulkanBE.sync.renderFinished = realloc(vulkanBE.sync.renderFinished, MAX_FRAME_DRAWS * sizeof(VkSemaphore));
    vulkanBE.sync.drawFences = realloc(vulkanBE.sync.drawFences, MAX_FRAME_DRAWS * sizeof(VkFence));

    //semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i=0; i<MAX_FRAME_DRAWS; i++)
    {
        if (vkCreateSemaphore(vulkanBE.mainDevices.device, &semaphoreCreateInfo, NULL, &vulkanBE.sync.imageAvailable[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(vulkanBE.mainDevices.device, &semaphoreCreateInfo, NULL, &vulkanBE.sync.renderFinished[i]) !=
            VK_SUCCESS ||
            vkCreateFence(vulkanBE.mainDevices.device, &fenceCreateInfo, NULL, &vulkanBE.sync.drawFences[i]))
        {
            fprintf(stderr, "failed to create semaphores and/or fences\n");
            exit(1);
        }
    }

}

static void cleanUpBackend()
{
    vkDeviceWaitIdle(vulkanBE.mainDevices.device);
    for(size_t i = 0; i< MAX_FRAME_DRAWS; i++)
    {
        vkDestroySemaphore(vulkanBE.mainDevices.device, vulkanBE.sync.renderFinished[i], NULL);
        vkDestroySemaphore(vulkanBE.mainDevices.device, vulkanBE.sync.imageAvailable[i], NULL);
        vkDestroyFence(vulkanBE.mainDevices.device, vulkanBE.sync.drawFences[i], NULL);
    }
    vkDestroyCommandPool(vulkanBE.mainDevices.device, vulkanBE.pools.graphicsCommandPool, NULL);
    for(int i = 0; i<vulkanBE.num_swapchain_images; i++)
    {
        vkDestroyFramebuffer(vulkanBE.mainDevices.device, vulkanBE.swapChainFrameBuffers[i], NULL);
    }
    vkDestroyPipeline(vulkanBE.mainDevices.device, vulkanBE.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(vulkanBE.mainDevices.device, vulkanBE.pipelineLayout, NULL);
    vkDestroyRenderPass(vulkanBE.mainDevices.device,vulkanBE.renderPass,NULL);
    for(int i = 0; i<vulkanBE.num_swapchain_images; i++)
    {
        vkDestroyImageView(vulkanBE.mainDevices.device, vulkanBE.swapChainImages[i].imageView, NULL);
    }
    vkDestroySwapchainKHR(vulkanBE.mainDevices.device,vulkanBE.swapChain,NULL);
    vkDestroySurfaceKHR(vulkanBE.instance, vulkanBE.surface, NULL);
    vkDestroyDevice(vulkanBE.mainDevices.device, NULL);
    if (enableValidationLayers) {DestroyDebugUtilsMessengerEXT(vulkanBE.instance, debugMessenger, NULL);}
    vkDestroyInstance(vulkanBE.instance, NULL);
    cleanUpVulkanBE();
}

static void initBackend(){

    initVulkanBE();

    //no try catch here so be careful
    initWindow("Hello",800,600);
    createInstance();
    setupDebugMessenger(vulkanBE.instance);
    createSurface();
    getPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffers();
    createSynchronization();
}