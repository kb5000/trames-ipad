// SPDX-License-Identifier: GPL-3.0-or-later
#include <vulkan/vk_android_native_buffer.h>
#include <fcntl.h>
#include <poll.h>
#include "buffer.h"
static VkResult android_usage(VkDevice device,VkFormat format,VkImageUsageFlags usage,int *out) {
    (void)device;(void)usage;
    if(format!=VK_FORMAT_R8G8B8A8_UNORM && format!=VK_FORMAT_B8G8R8A8_UNORM)return VK_ERROR_FORMAT_NOT_SUPPORTED;
    *out=0x100|0x200;return VK_SUCCESS; // texture | render, original Gralloc0 ABI
}
static VkResult android_create_image(VkDevice device,const VkImageCreateInfo *info,const VkAllocationCallbacks *allocator,VkImage *out) {
    const VkNativeBufferANDROID *native=find_next(info->pNext,VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID);
    if(!native)return vkCreateImage(device,info,allocator,out);
    const struct trames_buffer *buffer=trames_buffer(native->handle);
    if(!buffer || buffer->width!=info->extent.width || buffer->height!=info->extent.height || info->arrayLayers!=1 || info->mipLevels!=1)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    uint64_t values[6];VkResult result=CALL(273,buffer->fd,PTR(values));if(result)return result;
    if(values[3]!=(uint32_t)info->format)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    VkSubresourceLayout plane={.rowPitch=values[4]};
    VkImageDrmFormatModifierExplicitCreateInfoEXT modifier={.sType=VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,.drmFormatModifierPlaneCount=1,.pPlaneLayouts=&plane};
    VkExternalMemoryImageCreateInfo external={.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,.pNext=&modifier,.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
    VkImageCreateInfo create=*info;create.pNext=&external;create.tiling=VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    result=vkCreateImage(device,&create,allocator,out);if(result)return result;
    VkMemoryRequirements req;vkGetImageMemoryRequirements(device,*out,&req);
    int fd=fcntl(buffer->fd,F_DUPFD_CLOEXEC,0);if(fd<0){vkDestroyImage(device,*out,allocator);return VK_ERROR_OUT_OF_HOST_MEMORY;}
    VkMemoryDedicatedAllocateInfo dedicated={.sType=VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,.image=*out};
    VkImportMemoryFdInfoKHR imported={.sType=VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,.pNext=&dedicated,.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,.fd=fd};
    VkMemoryAllocateInfo allocate={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.pNext=&imported,.allocationSize=values[5],.memoryTypeIndex=__builtin_ctz(req.memoryTypeBits)};
    VkDeviceMemory memory;result=vkAllocateMemory(device,&allocate,NULL,&memory);
    if(result){close(fd);vkDestroyImage(device,*out,allocator);*out=VK_NULL_HANDLE;return result;}
    result=vkBindImageMemory(device,*out,memory,0);
    // The image's original native dependency retains storage through all uses.
    vkFreeMemory(device,memory,NULL);
    if(result){vkDestroyImage(device,*out,allocator);*out=VK_NULL_HANDLE;}
    return result;
}
static VkResult android_acquire(VkDevice device,VkImage image,int fd,VkSemaphore semaphore,VkFence fence) {
    (void)image;
    if(fd>=0){struct pollfd wait={fd,POLLIN,0};int result;do{result=poll(&wait,1,-1);}while(result<0 && errno==EINTR);int error=result<0 || wait.revents&(POLLERR|POLLNVAL);close(fd);if(error)return VK_ERROR_DEVICE_LOST;}
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.signalSemaphoreCount=semaphore?1:0,.pSignalSemaphores=&semaphore};
    return vkQueueSubmit((VkQueue)D(device)->queue,1,&submit,fence);
}
static VkResult android_release(VkQueue queue,uint32_t count,const VkSemaphore *semaphores,VkImage image,int *out) {
    (void)image;*out=-1;if(count>64)return VK_ERROR_FEATURE_NOT_PRESENT;
    VkDevice device=(VkDevice)DEVICE(queue);VkFence fence;VkFenceCreateInfo create={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkResult result=vkCreateFence(device,&create,NULL,&fence);if(result)return result;
    VkPipelineStageFlags stages[64];for(uint32_t i=0;i<count;i++)stages[i]=VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.waitSemaphoreCount=count,.pWaitSemaphores=semaphores,.pWaitDstStageMask=stages};
    result=vkQueueSubmit(queue,1,&submit,fence);
    // Initial compatibility path: the existing native wait parks this guest
    // worker until actual producer completion. -1 means completed, never a
    // fabricated signaled fence. No service thread or queue/device idle call.
    if(!result)result=vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX);
    vkDestroyFence(device,fence,NULL);return result;
}
static PFN_vkVoidFunction android_device_proc(VkDevice device,const char *name) {
    if(!device || !name)return NULL;
    if(!strcmp(name,"vkCreateImage"))return (PFN_vkVoidFunction)android_create_image;
    if(!strcmp(name,"vkGetSwapchainGrallocUsageANDROID"))return (PFN_vkVoidFunction)android_usage;
    if(!strcmp(name,"vkAcquireImageANDROID"))return (PFN_vkVoidFunction)android_acquire;
    if(!strcmp(name,"vkQueueSignalReleaseImageANDROID"))return (PFN_vkVoidFunction)android_release;
    return vkGetDeviceProcAddr(device,name);
}
static VkResult android_device_extensions(VkPhysicalDevice physical,const char *layer,uint32_t *count,VkExtensionProperties *out) {
    VkExtensionProperties properties[256];uint32_t size=256;VkResult result=vkEnumerateDeviceExtensionProperties(physical,layer,&size,properties);if(result)return result;
    uint32_t n=0;
    for(uint32_t i=0;i<size;i++)if(strcmp(properties[i].extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME) && strcmp(properties[i].extensionName,VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME))properties[n++]=properties[i];
    if(D(physical)->capabilities[0]&8192){properties[n]=(VkExtensionProperties){.specVersion=5};strcpy(properties[n++].extensionName,VK_ANDROID_NATIVE_BUFFER_EXTENSION_NAME);properties[n]=(VkExtensionProperties){.specVersion=5};strcpy(properties[n++].extensionName,VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME);}
    if(!out){*count=n;return VK_SUCCESS;}uint32_t capacity=*count;*count=capacity<n?capacity:n;memcpy(out,properties,*count*sizeof(*out));return capacity<n?VK_INCOMPLETE:VK_SUCCESS;
}
