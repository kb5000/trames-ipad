// SPDX-License-Identifier: GPL-3.0-or-later
// Android loader/HAL frontend over the same guest wire encoder as Linux.
// Android owns WSI/BufferQueue. Native image allocation, imports and completion
// share the Linux frontend's original guest-memory, FD and GPU resource model.
#define _GNU_SOURCE
#define VK_USE_PLATFORM_ANDROID_KHR
#include <android/hardware_buffer.h>
#include "../vulkan/icd.c"
#include <hardware/hwvulkan.h>
#include "native-buffer.inc"
#include "hardware-buffer.inc"

_Static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "dispatch ABI");
extern hwvulkan_module_t HAL_MODULE_INFO_SYM;
static VkResult android_instance_extensions(const char *layer,uint32_t *count,VkExtensionProperties *out) {
    VkExtensionProperties properties[32];uint32_t size=32;
    VkResult result=vkEnumerateInstanceExtensionProperties(layer,&size,properties);if(result)return result;
    uint32_t n=0;
    for(uint32_t i=0;i<size;i++)if(strcmp(properties[i].extensionName,VK_KHR_SURFACE_EXTENSION_NAME) && strcmp(properties[i].extensionName,VK_KHR_DISPLAY_EXTENSION_NAME) && strcmp(properties[i].extensionName,VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))properties[n++]=properties[i];
    if(!out){*count=n;return VK_SUCCESS;}uint32_t capacity=*count;*count=capacity<n?capacity:n;memcpy(out,properties,*count*sizeof(*out));return capacity<n?VK_INCOMPLETE:VK_SUCCESS;
}
static VkResult android_create_device(VkPhysicalDevice physical,const VkDeviceCreateInfo *info,const VkAllocationCallbacks *allocator,VkDevice *out) {
    const char *names[256];uint32_t count=0;
    if(info->enabledExtensionCount>256)return VK_ERROR_EXTENSION_NOT_PRESENT;
    for(uint32_t i=0;i<info->enabledExtensionCount;i++)if(strcmp(info->ppEnabledExtensionNames[i],VK_ANDROID_NATIVE_BUFFER_EXTENSION_NAME) && strcmp(info->ppEnabledExtensionNames[i],VK_ANDROID_EXTERNAL_MEMORY_ANDROID_HARDWARE_BUFFER_EXTENSION_NAME))names[count++]=info->ppEnabledExtensionNames[i];
    VkDeviceCreateInfo create=*info;create.enabledExtensionCount=count;create.ppEnabledExtensionNames=names;
    VkResult result=vkCreateDevice(physical,&create,allocator,out);
    if(result==VK_SUCCESS)D(*out)->parent=KEY(physical);
    if(getenv("TRAMES_ANDROID_TRACE"))fprintf(stderr,"Trames Android CreateDevice: physical=%p result=%d device=%p extensions=%u\n",(void*)physical,result,(void*)*out,info->enabledExtensionCount);
    return result;
}
// The Android profile exposes native family zero. Older Android ANGLE defers
// device creation for multiple graphics families, then uses its null device
// while initializing VMA. Queue indices within this native family are intact;
// the Linux frontend continues to expose all native families.
static void android_queue_families(VkPhysicalDevice physical,uint32_t *count,VkQueueFamilyProperties *out) {
    if(!out){*count=1;return;}
    if(!*count)return;
    *count=1;vkGetPhysicalDeviceQueueFamilyProperties(physical,count,out);
}
static void android_queue_families2(VkPhysicalDevice physical,uint32_t *count,VkQueueFamilyProperties2 *out) {
    if(!out){*count=1;return;}
    if(!*count)return;
    *count=1;vkGetPhysicalDeviceQueueFamilyProperties2(physical,count,out);
}
static PFN_vkVoidFunction android_proc(VkInstance instance,const char *name) {
    if(name && !strcmp(name,"vkGetInstanceProcAddr"))return (PFN_vkVoidFunction)android_proc;
    if(name && !strcmp(name,"vkGetDeviceProcAddr"))return (PFN_vkVoidFunction)android_hardware_proc;
    if(name && !strcmp(name,"vkEnumerateDeviceExtensionProperties"))return (PFN_vkVoidFunction)android_device_extensions;
    if(name && (!strcmp(name,"vkGetPhysicalDeviceImageFormatProperties2") || !strcmp(name,"vkGetPhysicalDeviceImageFormatProperties2KHR")))return (PFN_vkVoidFunction)android_hardware_format;
    if(name && !strcmp(name,"vkCreateDevice"))return (PFN_vkVoidFunction)android_create_device;
    if(name && !strcmp(name,"vkGetPhysicalDeviceQueueFamilyProperties"))return (PFN_vkVoidFunction)android_queue_families;
    if(name && (!strcmp(name,"vkGetPhysicalDeviceQueueFamilyProperties2") || !strcmp(name,"vkGetPhysicalDeviceQueueFamilyProperties2KHR")))return (PFN_vkVoidFunction)android_queue_families2;
    return vkGetInstanceProcAddr(instance,name);
}
static int android_close(hw_device_t *device) { (void)device;return 0; }
static hwvulkan_device_t android_device = {
    .common = {
        .tag = HARDWARE_DEVICE_TAG,
        .version = HWVULKAN_DEVICE_API_VERSION_0_1,
        .module = &HAL_MODULE_INFO_SYM.common,
        .close = android_close,
    },
    .EnumerateInstanceExtensionProperties = android_instance_extensions,
    .CreateInstance = vkCreateInstance,
    .GetInstanceProcAddr = android_proc,
};
static int android_open(const hw_module_t *module,const char *id,hw_device_t **device) {
    (void)module;
    if(!id || strcmp(id,HWVULKAN_DEVICE_0) || !device)return -EINVAL;
    *device=&android_device.common;return 0;
}
static hw_module_methods_t android_methods = {.open = android_open};
__attribute__((visibility("default"))) hwvulkan_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = HWVULKAN_HARDWARE_MODULE_ID,
        .name = "Trames native Vulkan bridge",
        .author = "Trames",
        .methods = &android_methods,
    },
};
