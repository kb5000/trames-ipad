// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef DEVICE_NATIVE_GPU
#include <vulkan/vulkan.h>
#else
#include <sys/eventfd.h>
extern int64_t gpu_call(uint64_t, const uint64_t *, uint64_t);
extern int64_t video_call(uint64_t, const uint64_t *, uint64_t);
#endif
static pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t started=PTHREAD_COND_INITIALIZER;
static unsigned participants;
static atomic_int finished;
static unsigned long peer_calls;
static void start_together(void) {
    assert(!pthread_mutex_lock(&gate));
    if(++participants==2) assert(!pthread_cond_broadcast(&started));
    while(participants<2) assert(!pthread_cond_wait(&started,&gate));
    assert(!pthread_mutex_unlock(&gate));
}
static double seconds(void) {
    struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC, &t));
    return t.tv_sec + t.tv_nsec * 1e-9;
}
__attribute__((noinline)) static void fill(unsigned char *p,unsigned char value) {
    memset(p,value,65536);
    __asm__ volatile("" : : "r"(p) : "memory");
}
static void *peer(void *unused) {
    (void)unused;
    unsigned char *p=malloc(65536); assert(p);
    memset(p, 0, 65536);
    start_together();
    unsigned long n=0;
    do {
        fill(p,(unsigned char)n);
        assert(p[0]==(unsigned char)n && p[65535]==(unsigned char)n);
        ++n;
    } while(!atomic_load_explicit(&finished, memory_order_relaxed));
    peer_calls=n; free(p); return NULL;
}
#ifndef DEVICE_NATIVE_GPU
static int64_t call(uint64_t profile,uint64_t op,uint64_t a,uint64_t b,uint64_t c) {
    uint64_t record[16]={a,b,c};
    return profile==0x10006 ? gpu_call(op,record,sizeof record) : video_call(op,record,sizeof record);
}
#define P(x) ((uint64_t)(uintptr_t)(x))
#endif
static void gpu_cycle(void) {
#ifdef DEVICE_NATIVE_GPU
    VkInstance instance=VK_NULL_HANDLE;
    VkInstanceCreateInfo ci={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    // Direct static MoltenVK has no loader portability-enumeration extension.
    VkApplicationInfo info={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion=VK_API_VERSION_1_2};
    ci.pApplicationInfo=&info;

    assert(vkCreateInstance(&ci,NULL,&instance)==VK_SUCCESS);
    uint32_t count=1;VkPhysicalDevice physical=VK_NULL_HANDLE;
    assert(vkEnumeratePhysicalDevices(instance,&count,&physical)==VK_SUCCESS && count==1);
    float priority=1;
    VkDeviceQueueCreateInfo queue={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex=0,.queueCount=1,.pQueuePriorities=&priority};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount=1,.pQueueCreateInfos=&queue};
    VkDevice device=VK_NULL_HANDLE;assert(vkCreateDevice(physical,&di,NULL,&device)==VK_SUCCESS);
    assert(vkDeviceWaitIdle(device)==VK_SUCCESS);
    vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);
#else
    uint64_t instance=0,physical=0,device=0;uint32_t count=1;
    assert(!call(0x10006,1,P(&instance),0,0));
    assert(!call(0x10006,3,instance,P(&count),P(&physical)) && count==1);
    assert(!call(0x10006,8,physical,0,P(&device)));
    assert(!call(0x10006,9,device,0,0));
    assert(!call(0x10006,2,instance,0,0));
#endif
}
int main(int argc,char **argv) {
    assert(argc==3);int gpu=!strcmp(argv[1],"gpu");
    assert(gpu || !strcmp(argv[1],"video"));
    unsigned count=(unsigned)strtoul(argv[2],NULL,10);assert(count && count<=100000);
#ifdef DEVICE_NATIVE_GPU
    assert(gpu);
#else
    // Create the notification descriptor after the executor captured its Files
    // authority. No decoder or microphone is needed for surface metadata.
    int fd=eventfd(0,EFD_NONBLOCK);assert(fd>=0);
#endif
    pthread_t thread;assert(!pthread_create(&thread,NULL,peer,NULL));
    start_together();
    // Both native and Trames initialize the original GPU driver before timing.
    if(gpu)gpu_cycle();
    double start=seconds();
    for(unsigned i=0;i<count;i++) {
        if(gpu)gpu_cycle();
#ifndef DEVICE_NATIVE_GPU
        else {
            int64_t surface=call(0x10007,3,(unsigned)fd,1920,1080);assert(surface>0);
            assert(!call(0x10007,4,(uint64_t)surface,0,0));
        }
#endif
    }
    double elapsed=seconds()-start;
    atomic_store_explicit(&finished,1,memory_order_relaxed);
    assert(!pthread_join(thread,NULL));assert(peer_calls>0);
#ifndef DEVICE_NATIVE_GPU
    assert(!close(fd));
#endif
    printf("{\"mode\":\"%s\",\"operations\":%u,\"seconds\":%.9f,\"peer_calls\":%lu}\n",
        argv[1],count,elapsed,peer_calls);
    return 0;
}
