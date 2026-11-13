// SPDX-License-Identifier: GPL-3.0-or-later
// Original libnativewindow owns AHardwareBuffer/GraphicBuffer references.
static const struct trames_buffer *android_buffer_handle(const AHardwareBuffer *buffer) {
    typedef const native_handle_t *(*GetHandle)(const AHardwareBuffer *);
    void *library=dlopen("libnativewindow.so",RTLD_NOW|RTLD_LOCAL);if(!library)return NULL;
    GetHandle get=(GetHandle)dlsym(library,"AHardwareBuffer_getNativeHandle");
    const struct trames_buffer *handle=get?trames_buffer(get(buffer)):NULL;dlclose(library);return handle;
}
static VkResult android_buffer_properties(VkDevice device,const AHardwareBuffer *buffer,VkAndroidHardwareBufferPropertiesANDROID *out) {
    if(!buffer || !out || out->sType!=VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    const struct trames_buffer *handle=android_buffer_handle(buffer);if(!handle)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    uint64_t info[6];VkResult result=CALL(273,handle->fd,PTR(info));if(result)return result;
    if(info[3]!=37 && info[3]!=44)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    VkMemoryFdPropertiesKHR properties={.sType=VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};result=vkGetMemoryFdPropertiesKHR(device,VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,handle->fd,&properties);if(result)return result;
    out->allocationSize=info[5];out->memoryTypeBits=properties.memoryTypeBits;
    VkFormatProperties format;checked(CALL(58,D(device)->parent,(VkFormat)info[3],PTR(&format)));
    // Format queries are physical-device operations; use the instance's driver
    // metadata below via the matching retained physical dispatch.
    for(VkBaseOutStructure *next=out->pNext;next;next=next->pNext) {
        if(next->sType==VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID) {
            VkAndroidHardwareBufferFormatPropertiesANDROID *v=(void*)next;
            v->format=(VkFormat)info[3];v->externalFormat=0;v->formatFeatures=format.optimalTilingFeatures;
            v->samplerYcbcrConversionComponents=(VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY};
            v->suggestedYcbcrModel=VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY;v->suggestedYcbcrRange=VK_SAMPLER_YCBCR_RANGE_ITU_FULL;v->suggestedXChromaOffset=v->suggestedYChromaOffset=VK_CHROMA_LOCATION_MIDPOINT;
        }
    }
    return VK_SUCCESS;
}
static VkResult android_hardware_allocate(VkDevice device,const VkMemoryAllocateInfo *info,const VkAllocationCallbacks *allocator,VkDeviceMemory *out) {
    const VkImportAndroidHardwareBufferInfoANDROID *import=find_next(info->pNext,VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID);
    if(!import || !import->buffer)return vkAllocateMemory(device,info,allocator,out);
    const struct trames_buffer *handle=android_buffer_handle(import->buffer);
    const VkMemoryDedicatedAllocateInfo *dedicated=find_next(info->pNext,VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO);
    if(!handle || !dedicated || !dedicated->image || dedicated->buffer)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    VkResult result=CALL(275,KEY(device),(uint64_t)dedicated->image,handle->fd);if(result)return result;
    int fd=fcntl(handle->fd,F_DUPFD_CLOEXEC,0);if(fd<0)return VK_ERROR_OUT_OF_HOST_MEMORY;
    VkImportMemoryFdInfoKHR imported={.sType=VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,.pNext=dedicated,.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,.fd=fd};
    VkMemoryAllocateInfo create=*info;create.pNext=&imported;if(!create.allocationSize)create.allocationSize=handle->size;
    result=vkAllocateMemory(device,&create,allocator,out);if(result){close(fd);return result;}
    AHardwareBuffer_acquire(import->buffer);pthread_mutex_lock(&D(device)->lock);Memory *record=memory(D(device),*out);record->android_buffer=import->buffer;pthread_mutex_unlock(&D(device)->lock);return VK_SUCCESS;
}
static void android_hardware_free(VkDevice device,VkDeviceMemory value,const VkAllocationCallbacks *allocator) {
    pthread_mutex_lock(&D(device)->lock);Memory *record=memory(D(device),value);AHardwareBuffer *buffer=record?record->android_buffer:NULL;if(record)record->android_buffer=NULL;pthread_mutex_unlock(&D(device)->lock);
    vkFreeMemory(device,value,allocator);if(buffer)AHardwareBuffer_release(buffer);
}
static VkResult android_hardware_export(VkDevice device,const VkMemoryGetAndroidHardwareBufferInfoANDROID *info,AHardwareBuffer **out) {
    if(!info || info->sType!=VK_STRUCTURE_TYPE_MEMORY_GET_ANDROID_HARDWARE_BUFFER_INFO_ANDROID)return VK_ERROR_INVALID_EXTERNAL_HANDLE;
    pthread_mutex_lock(&D(device)->lock);Memory *record=memory(D(device),info->memory);
    if(!record || !record->android_buffer){pthread_mutex_unlock(&D(device)->lock);return VK_ERROR_INVALID_EXTERNAL_HANDLE;}
    *out=record->android_buffer;AHardwareBuffer_acquire(*out);pthread_mutex_unlock(&D(device)->lock);return VK_SUCCESS;
}
static VkResult android_hardware_image(VkDevice device,const VkImageCreateInfo *info,const VkAllocationCallbacks *allocator,VkImage *out) {
    const VkExternalMemoryImageCreateInfo *external=find_next(info->pNext,VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO);
    if(!external || external->handleTypes!=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)return android_create_image(device,info,allocator,out);
    if(info->format!=VK_FORMAT_R8G8B8A8_UNORM && info->format!=VK_FORMAT_B8G8R8A8_UNORM)return VK_ERROR_FORMAT_NOT_SUPPORTED;
    VkSubresourceLayout plane={.rowPitch=info->extent.width*4ull};
    VkImageDrmFormatModifierExplicitCreateInfoEXT modifier={.sType=VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,.drmFormatModifierPlaneCount=1,.pPlaneLayouts=&plane};
    VkExternalMemoryImageCreateInfo translated={.sType=VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,.pNext=&modifier,.handleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
    VkImageCreateInfo create=*info;create.pNext=&translated;create.tiling=VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    return vkCreateImage(device,&create,allocator,out);
}
static VkResult android_hardware_format(VkPhysicalDevice physical,const VkPhysicalDeviceImageFormatInfo2 *info,VkImageFormatProperties2 *out) {
    const VkPhysicalDeviceExternalImageFormatInfo *external=find_next(info->pNext,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO);
    if(!external || external->handleType!=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)return vkGetPhysicalDeviceImageFormatProperties2(physical,info,out);
    if(info->format!=VK_FORMAT_R8G8B8A8_UNORM && info->format!=VK_FORMAT_B8G8R8A8_UNORM)return VK_ERROR_FORMAT_NOT_SUPPORTED;
    VkPhysicalDeviceExternalImageFormatInfo translated=*external;translated.pNext=NULL;translated.handleType=VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkPhysicalDeviceImageFormatInfo2 create=*info;create.pNext=&translated;
    VkResult result=vkGetPhysicalDeviceImageFormatProperties2(physical,&create,out);if(result)return result;
    for(VkBaseOutStructure *next=out->pNext;next;next=next->pNext) {
        if(next->sType==VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES){VkExternalImageFormatProperties *v=(void*)next;v->externalMemoryProperties.compatibleHandleTypes=VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;v->externalMemoryProperties.exportFromImportedHandleTypes=0;}
        if(next->sType==VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID){VkAndroidHardwareBufferUsageANDROID *v=(void*)next;v->androidHardwareBufferUsage=AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE|AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT;}
    }
    return VK_SUCCESS;
}
static PFN_vkVoidFunction android_hardware_proc(VkDevice device,const char *name) {
    if(!device || !name)return NULL;
    if(!strcmp(name,"vkGetDeviceProcAddr"))return (PFN_vkVoidFunction)android_hardware_proc;
    if(!strcmp(name,"vkGetAndroidHardwareBufferPropertiesANDROID"))return (PFN_vkVoidFunction)android_buffer_properties;
    if(!strcmp(name,"vkGetMemoryAndroidHardwareBufferANDROID"))return (PFN_vkVoidFunction)android_hardware_export;
    if(!strcmp(name,"vkAllocateMemory"))return (PFN_vkVoidFunction)android_hardware_allocate;
    if(!strcmp(name,"vkFreeMemory"))return (PFN_vkVoidFunction)android_hardware_free;
    if(!strcmp(name,"vkCreateImage"))return (PFN_vkVoidFunction)android_hardware_image;
    return android_device_proc(device,name);
}
