// SPDX-License-Identifier: GPL-3.0-or-later
// Android loader/HAL frontend over the same guest wire encoder as Linux.
// BufferQueue/Gralloc interoperability is a separate capability; this module
// does not advertise Android native-buffer or Android surface extensions.
#include "../vulkan/icd.c"
#include <hardware/hwvulkan.h>

_Static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "dispatch ABI");
extern hwvulkan_module_t HAL_MODULE_INFO_SYM;
static int android_close(hw_device_t *device) { (void)device;return 0; }
static hwvulkan_device_t android_device = {
    .common = {
        .tag = HARDWARE_DEVICE_TAG,
        .version = HWVULKAN_DEVICE_API_VERSION_0_1,
        .module = &HAL_MODULE_INFO_SYM.common,
        .close = android_close,
    },
    .EnumerateInstanceExtensionProperties = vkEnumerateInstanceExtensionProperties,
    .CreateInstance = vkCreateInstance,
    .GetInstanceProcAddr = vkGetInstanceProcAddr,
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
