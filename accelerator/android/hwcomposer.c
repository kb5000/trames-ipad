// SPDX-License-Identifier: GPL-3.0-or-later
// Public HWC1 ABI wrapper. All composition, buffers and input remain upstream.
#include <hardware/hwcomposer.h>
#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "buffer.h"
#include "hwc-crop.h"
#include "hwc-events.h"
struct adapter {
    hwc_procs_t callbacks;
    const hwc_procs_t *client;
    hwc_composer_device_1_t *device;
    void (*register_procs)(hwc_composer_device_1_t *,const hwc_procs_t *);
    int (*control)(hwc_composer_device_1_t *,int,int,int);
    int (*set)(hwc_composer_device_1_t *,size_t,hwc_display_contents_1_t **);
    void (*dump)(hwc_composer_device_1_t *,char *,int);
    atomic_ulong resize_frames,reconnects;
    struct trames_hwc_events events;
};
static struct adapter adapter={.events={.lock=PTHREAD_MUTEX_INITIALIZER}};
static struct adapter *from_callbacks(const hwc_procs_t *p) {
    return (struct adapter *)((char *)p - offsetof(struct adapter,callbacks));
}
static void enable(void *owner,bool wanted) {
    struct adapter *a=owner;a->control(a->device,HWC_DISPLAY_PRIMARY,HWC_EVENT_VSYNC,wanted);
}
static void reconnect(void *owner) {
    struct adapter *a=owner;
    atomic_fetch_add_explicit(&a->reconnects,1,memory_order_relaxed);
    if(a->client->hotplug)a->client->hotplug(a->client,HWC_DISPLAY_PRIMARY,1);
}
static void vsync(void *owner,int64_t time) {
    struct adapter *a=owner;
    if(a->client->vsync)a->client->vsync(a->client,HWC_DISPLAY_PRIMARY,time);
}
static void on_invalidate(const hwc_procs_t *p) {
    struct adapter *a=from_callbacks(p);if(a->client->invalidate)a->client->invalidate(a->client);
}
static void on_vsync(const hwc_procs_t *p,int display,int64_t time) {
    struct adapter *a=from_callbacks(p);
    if(display==HWC_DISPLAY_PRIMARY)trames_hwc_vsync(&a->events,time);
    else if(a->client->vsync)a->client->vsync(a->client,display,time);
}
static void on_hotplug(const hwc_procs_t *p,int display,int connected) {
    struct adapter *a=from_callbacks(p);
    if(display==HWC_DISPLAY_PRIMARY && connected)trames_hwc_request(&a->events);
    else if(a->client->hotplug)a->client->hotplug(a->client,display,connected);
}
static void register_procs(hwc_composer_device_1_t *device,const hwc_procs_t *client) {
    adapter.client=client;
    adapter.callbacks=(hwc_procs_t){.invalidate=on_invalidate,.vsync=on_vsync,.hotplug=on_hotplug};
    adapter.register_procs(device,&adapter.callbacks);
}
static int control(hwc_composer_device_1_t *device,int display,int event,int value) {
    if(display==HWC_DISPLAY_PRIMARY && event==HWC_EVENT_VSYNC && (value==0 || value==1)) {
        trames_hwc_enable(&adapter.events,value!=0);return 0;
    }
    return adapter.control(device,display,event,value);
}
struct crop_fix {hwc_layer_1_t *layer;hwc_frect_t saved;};
static bool clipped(hwc_layer_1_t *layer,hwc_frect_t *out) {
    const struct trames_buffer *buffer=trames_buffer(layer->handle);
    if(!buffer)return false;
    hwc_frect_t c=layer->sourceCropf;float value[4]={c.left,c.top,c.right,c.bottom};
    if(!trames_hwc_crop(value,buffer->width,buffer->height))return false;
    *out=(hwc_frect_t){value[0],value[1],value[2],value[3]};return true;
}
static int set(hwc_composer_device_1_t *device,size_t count,hwc_display_contents_1_t **displays) {
    size_t changed=0;hwc_frect_t crop;
    for(size_t d=0;d<count;d++)if(displays[d])for(size_t i=0;i<displays[d]->numHwLayers;i++)
        changed+=clipped(&displays[d]->hwLayers[i],&crop);
    if(!changed)return adapter.set(device,count,displays);
    atomic_fetch_add_explicit(&adapter.resize_frames,1,memory_order_relaxed);
    // Allocate only for mismatched resize frames. Restore the caller's geometry
    // afterwards so a temporary old buffer cannot truncate later full-size frames.
    struct crop_fix *fixes=malloc(changed*sizeof(*fixes));if(!fixes)return -ENOMEM;
    size_t n=0;
    for(size_t d=0;d<count;d++)if(displays[d])for(size_t i=0;i<displays[d]->numHwLayers;i++) {
        hwc_layer_1_t *layer=&displays[d]->hwLayers[i];
        if(clipped(layer,&crop)){fixes[n++]=(struct crop_fix){layer,layer->sourceCropf};layer->sourceCropf=crop;}
    }
    int result=adapter.set(device,count,displays);
    for(size_t i=0;i<n;i++)fixes[i].layer->sourceCropf=fixes[i].saved;
    free(fixes);return result;
}
static void dump(hwc_composer_device_1_t *device,char *buffer,int length) {
    if(length<=0)return;
    int used=snprintf(buffer,(size_t)length,"Trames HWC: resize_crop_frames=%lu async_hotplugs=%lu\n",
        atomic_load_explicit(&adapter.resize_frames,memory_order_relaxed),atomic_load_explicit(&adapter.reconnects,memory_order_relaxed));
    if(adapter.dump && used>=0 && used<length)adapter.dump(device,buffer+used,length-used);
}
static int open_device(const hw_module_t *module,const char *name,hw_device_t **out) {
    (void)module;
    // The selected Waydroid HWC1 owns one device and one existing vsync thread.
    if(adapter.device)return -EBUSY;
    void *library=dlopen("/vendor/lib64/hw/hwcomposer.waydroid.so",RTLD_NOW|RTLD_LOCAL);
    if(!library)return -ENOENT;
    hw_module_t *upstream=dlsym(library,HAL_MODULE_INFO_SYM_AS_STR);
    if(!upstream || !upstream->methods || !upstream->methods->open)return -EINVAL;
    int result=upstream->methods->open(upstream,name,out);if(result)return result;
    adapter.device=(hwc_composer_device_1_t *)*out;
    adapter.register_procs=adapter.device->registerProcs;adapter.control=adapter.device->eventControl;adapter.set=adapter.device->set;adapter.dump=adapter.device->dump;
    adapter.events.owner=&adapter;adapter.events.enable=enable;adapter.events.reconnect=reconnect;adapter.events.vsync=vsync;
    atomic_init(&adapter.events.wanted,true);
    adapter.device->registerProcs=register_procs;adapter.device->eventControl=control;adapter.device->set=set;adapter.device->dump=dump;
    return 0;
}
static hw_module_methods_t methods={.open=open_device};
hwc_module_t HAL_MODULE_INFO_SYM={.common={.tag=HARDWARE_MODULE_TAG,.module_api_version=HWC_MODULE_API_VERSION_0_1,.hal_api_version=HARDWARE_HAL_API_VERSION,.id=HWC_HARDWARE_MODULE_ID,.name="Trames Waydroid callback adapter",.author="Trames",.methods=&methods}};
