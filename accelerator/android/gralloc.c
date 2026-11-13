// SPDX-License-Identifier: GPL-3.0-or-later
// Original Android allocator/mapper adapters wrap this small Gralloc0 HAL.
#include <hardware/gralloc.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>
#include "buffer.h"
struct result {uint64_t value,fallback;};
extern struct result trames_vulkan_call(uint64_t,const uint64_t *,size_t);
static int bridge(unsigned op,uint64_t args[16]) {return (int32_t)trames_vulkan_call(op,args,128).value;}
static int describe(buffer_handle_t handle,uint64_t info[6]) {
    const struct trames_buffer *b=trames_buffer(handle);if(!b)return -EINVAL;
    return bridge(273,(uint64_t[16]){b->fd,(uintptr_t)info})? -EINVAL:0;
}
static int reg(const gralloc_module_t *module,buffer_handle_t handle) {
    (void)module;uint64_t info[6];if(describe(handle,info))return -EINVAL;
    struct trames_buffer *b=(void*)handle;
    uint32_t vkformat=b->format==HAL_PIXEL_FORMAT_BLOB?9:b->format==HAL_PIXEL_FORMAT_BGRA_8888?44:37;
    if(b->width!=info[1] || b->height!=info[2] || vkformat!=info[3] || b->stride*(vkformat==9?1ull:4ull)!=info[4] || b->size!=info[5])return -EINVAL;
    b->address=0;return 0;
}
static int unreg(const gralloc_module_t *module,buffer_handle_t handle) {
    (void)module;struct trames_buffer *b=(void*)trames_buffer(handle);if(!b)return -EINVAL;
    if(b->address){munmap((void*)(uintptr_t)b->address,b->size);b->address=0;}return 0;
}
static int lock_buffer(const gralloc_module_t *module,buffer_handle_t handle,int usage,int left,int top,int width,int height,void **out) {
    (void)module;(void)usage;struct trames_buffer *b=(void*)trames_buffer(handle);
    if(!b || left<0 || top<0 || width<0 || height<0 || (uint64_t)left+width>b->width || (uint64_t)top+height>b->height || !out)return -EINVAL;
    if(!b->address){void *p=mmap(NULL,b->size,PROT_READ|PROT_WRITE,MAP_SHARED,b->fd,0);if(p==MAP_FAILED)return -errno;b->address=(uintptr_t)p;}
    if(bridge(274,(uint64_t[16]){b->fd,1}))return -EIO;
    *out=(void*)(uintptr_t)b->address;return 0;
}
static int unlock_buffer(const gralloc_module_t *module,buffer_handle_t handle) {
    (void)module;const struct trames_buffer *b=trames_buffer(handle);if(!b)return -EINVAL;
    return bridge(274,(uint64_t[16]){b->fd,0})?-EIO:0;
}
static int wait_fence(int fd) {
    if(fd==-1)return 0;
    if(fd<0)return -EINVAL;
    struct pollfd p={.fd=fd,.events=POLLIN};int result;
    do{result=poll(&p,1,-1);}while(result<0&&errno==EINTR);
    int error=result<0?-errno:!(p.revents&POLLIN)?-EINVAL:0;
    close(fd);return error;
}
static int lock_async(const gralloc_module_t *module,buffer_handle_t handle,int usage,int left,int top,int width,int height,void **out,int fence) {
    int error=wait_fence(fence);return error?error:lock_buffer(module,handle,usage,left,top,width,height,out);
}
static int unlock_async(const gralloc_module_t *module,buffer_handle_t handle,int *fence) {
    if(!fence)return -EINVAL;
    *fence=-1;return unlock_buffer(module,handle);
}
static int lock_ycbcr(const gralloc_module_t *module,buffer_handle_t handle,int usage,int left,int top,int width,int height,struct android_ycbcr *out) {
    (void)module;(void)handle;(void)usage;(void)left;(void)top;(void)width;(void)height;(void)out;return -EINVAL;
}
static int lock_async_ycbcr(const gralloc_module_t *module,buffer_handle_t handle,int usage,int left,int top,int width,int height,struct android_ycbcr *out,int fence) {
    int error=wait_fence(fence);return error?error:lock_ycbcr(module,handle,usage,left,top,width,height,out);
}
static int transport(const gralloc_module_t *module,buffer_handle_t handle,uint32_t *fds,uint32_t *ints) {
    (void)module;const struct trames_buffer *b=trames_buffer(handle);if(!b)return -EINVAL;*fds=b->base.numFds;*ints=b->base.numInts;return 0;
}
static int validate(const gralloc_module_t *module,buffer_handle_t handle,uint32_t width,uint32_t height,int32_t format,int usage,uint32_t stride) {
    (void)module;(void)usage;const struct trames_buffer *b=trames_buffer(handle);
    return b && b->width==width && b->height==height && b->format==(uint32_t)format && b->stride==stride?0:-EINVAL;
}
static int alloc_buffer(alloc_device_t *device,int width,int height,int format,int usage,buffer_handle_t *out,int *stride) {
    (void)device;if(width<=0 || height<=0 || usage&GRALLOC_USAGE_PROTECTED)return -EINVAL;
    uint32_t vkformat;
    switch(format){case HAL_PIXEL_FORMAT_RGBA_8888:case HAL_PIXEL_FORMAT_RGBX_8888:vkformat=37;break;case HAL_PIXEL_FORMAT_BGRA_8888:vkformat=44;break;case HAL_PIXEL_FORMAT_BLOB:if(height!=1)return -EINVAL;vkformat=9;break;default:return -EINVAL;}
    struct trames_buffer *b=calloc(1,sizeof(*b));if(!b)return -ENOMEM;
    uint64_t info[6];if(bridge(272,(uint64_t[16]){vkformat,width,height,(uintptr_t)info})){free(b);return -ENOMEM;}
    b->base.version=sizeof(native_handle_t);b->base.numFds=1;b->base.numInts=(sizeof(*b)-sizeof(native_handle_t)-sizeof(int))/sizeof(int);
    b->fd=info[0];b->magic=TRAMES_BUFFER_MAGIC;b->width=width;b->height=height;b->format=format;b->stride=info[4]/(vkformat==9?1:4);b->size=info[5];
    *out=&b->base;*stride=b->stride;return 0;
}
static int free_buffer(alloc_device_t *device,buffer_handle_t handle) {
    (void)device;struct trames_buffer *b=(void*)trames_buffer(handle);if(!b)return -EINVAL;unreg(NULL,handle);close(b->fd);free(b);return 0;
}
static int close_device(hw_device_t *device){free(device);return 0;}
static int open_device(const hw_module_t *module,const char *id,hw_device_t **out) {
    if(strcmp(id,GRALLOC_HARDWARE_GPU0))return -EINVAL;
    alloc_device_t *device=calloc(1,sizeof(*device));if(!device)return -ENOMEM;
    device->common=(hw_device_t){.tag=HARDWARE_DEVICE_TAG,.version=0,.module=(void*)module,.close=close_device};device->alloc=alloc_buffer;device->free=free_buffer;*out=&device->common;return 0;
}
static hw_module_methods_t methods={.open=open_device};
__attribute__((visibility("default"))) gralloc_module_t HAL_MODULE_INFO_SYM={
    .common={.tag=HARDWARE_MODULE_TAG,.module_api_version=GRALLOC_MODULE_API_VERSION_0_3,.hal_api_version=HARDWARE_HAL_API_VERSION,.id=GRALLOC_HARDWARE_MODULE_ID,.name="Trames native image allocator",.author="Trames",.methods=&methods},
    .registerBuffer=reg,.unregisterBuffer=unreg,.lock=lock_buffer,.unlock=unlock_buffer,.getTransportSize=transport,.validateBufferSize=validate,
    .lockAsync=lock_async,.unlockAsync=unlock_async,.lock_ycbcr=lock_ycbcr,.lockAsync_ycbcr=lock_async_ycbcr,
};
