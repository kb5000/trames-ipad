// SPDX-License-Identifier: GPL-3.0-or-later
// Direct HAL diagnostics complement the original Android libvulkan acceptance.
#include <hardware/hwvulkan.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <sys/auxv.h>
int main(void) {
    printf("Trames Vulkan auxv: %#lx\n",getauxval(0x54520008));
    void *support=dlopen("libvndksupport.so",RTLD_NOW|RTLD_LOCAL);
    if(!support){fprintf(stderr,"VNDK support: %s\n",dlerror());return 1;}
    void *(*load)(const char*,int)=dlsym(support,"android_load_sphal_library");
    int (*unload)(void*)=dlsym(support,"android_unload_sphal_library");
    if(!load||!unload){fprintf(stderr,"SPHAL entry points: %s\n",dlerror());return 1;}
    void *library=load("vulkan.trames.so",RTLD_NOW|RTLD_LOCAL);
    if(!library){fprintf(stderr,"HAL dlopen: %s\n",dlerror());return 1;}
    hwvulkan_module_t *module=dlsym(library,HAL_MODULE_INFO_SYM_AS_STR);
    if(!module){fprintf(stderr,"HAL HMI: %s\n",dlerror());return 1;}
    printf("HAL module: %s %s tag=%#x\n",module->common.id,module->common.name,module->common.tag);
    hwvulkan_device_t *device=NULL;
    int opened=module->common.methods->open(&module->common,HWVULKAN_DEVICE_0,(hw_device_t**)&device);
    if(opened||!device){fprintf(stderr,"HAL open: %d\n",opened);return 1;}
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.pApplicationName="Android HAL diagnostic",.apiVersion=VK_API_VERSION_1_0};
    VkInstanceCreateInfo info={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app};VkInstance instance;
    VkResult result=device->CreateInstance(&info,NULL,&instance);printf("HAL CreateInstance: %d\n",result);if(result)return 1;
    PFN_vkEnumeratePhysicalDevices enumerate=(PFN_vkEnumeratePhysicalDevices)device->GetInstanceProcAddr(instance,"vkEnumeratePhysicalDevices");
    PFN_vkDestroyInstance destroy=(PFN_vkDestroyInstance)device->GetInstanceProcAddr(instance,"vkDestroyInstance");
    uint32_t count=0;result=enumerate(instance,&count,NULL);printf("HAL physical devices: result=%d count=%u\n",result,count);destroy(instance,NULL);device->common.close(&device->common);unload(library);dlclose(support);
    return result!=VK_SUCCESS||count==0;
}
