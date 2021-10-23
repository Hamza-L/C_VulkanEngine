# C_VulkanEngine
Coding my very own Graphics Engine focusing on function and simpler architecture. It is coded in C and makes use of mostly static functions.
ve_window takes care of initializing the window
ve_backend takes care of the initialization portion of the instance, surface, logical device, graphics pipeline, renderpass, swapchain etc...
ve_renderer takes care of mesh objects, their creation/destruction as well as the buffer memory management it entails.
utilities.h provides some useful functions. Mainly SPIR-V file import for now.
