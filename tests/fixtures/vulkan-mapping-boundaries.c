// SPDX-License-Identifier: GPL-3.0-or-later
// Same-source native/guest Vulkan boundary-cost probe (256 MiB mapping).
// Keep a large coherent GPU allocation live across unrelated VM layout changes.
// Identical native/guest public Vulkan and mprotect calls; check actual GPU output.
#include <vulkan/vulkan.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#define OK(x) do { VkResult r=(x); if(r!=VK_SUCCESS){fprintf(stderr,"%s: %d\n",#x,r);abort();} } while(0)
static double seconds(void){struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return t.tv_sec+t.tv_nsec/1e9;}
static uint32_t memory_type(VkPhysicalDevice physical,uint32_t bits){
 VkPhysicalDeviceMemoryProperties p;vkGetPhysicalDeviceMemoryProperties(physical,&p);
 for(uint32_t i=0;i<p.memoryTypeCount;i++)if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))return i;
 abort();
}
static void run(VkPhysicalDevice physical,VkDevice device,VkQueue queue,uint32_t family,unsigned mode,unsigned iterations){
 const VkDeviceSize bytes=256u*1024*1024,tail=0; (void)mode;
 VkBuffer buffer;VkBufferCreateInfo bc={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=bytes,.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT};OK(vkCreateBuffer(device,&bc,NULL,&buffer));
 VkMemoryRequirements req;vkGetBufferMemoryRequirements(device,buffer,&req);VkDeviceSize allocation=mode==2?req.size:((req.size+16383)&~(VkDeviceSize)16383)+tail;
 VkMemoryDedicatedAllocateInfo dedicated={.sType=VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,.buffer=buffer};
 VkMemoryAllocateInfo ai={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.pNext=mode==2?&dedicated:NULL,.allocationSize=allocation,.memoryTypeIndex=memory_type(physical,req.memoryTypeBits)};
 VkDeviceMemory memory;OK(vkAllocateMemory(device,&ai,NULL,&memory));OK(vkBindBufferMemory(device,buffer,memory,0));void *mapped;OK(vkMapMemory(device,memory,0,VK_WHOLE_SIZE,0,&mapped));memset(mapped,0,(size_t)allocation);
 size_t page=(size_t)sysconf(_SC_PAGESIZE); void *scratch=mmap(NULL,page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(scratch!=MAP_FAILED);
 VkCommandPool pool;VkCommandPoolCreateInfo pc={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.queueFamilyIndex=family};OK(vkCreateCommandPool(device,&pc,NULL,&pool));
 VkCommandBuffer cb;VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1};OK(vkAllocateCommandBuffers(device,&ca,&cb));
 VkQueryPool query;VkQueryPoolCreateInfo qc={.sType=VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,.queryType=VK_QUERY_TYPE_TIMESTAMP,.queryCount=2};OK(vkCreateQueryPool(device,&qc,NULL,&query));
 VkCommandBufferBeginInfo begin={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};OK(vkBeginCommandBuffer(cb,&begin));vkCmdResetQueryPool(cb,query,0,2);vkCmdWriteTimestamp(cb,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,query,0);vkCmdFillBuffer(cb,buffer,bytes-4096,4096,0x7a35b19e);
 VkBufferMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT,.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED,.buffer=buffer,.offset=bytes-4096,.size=4096};vkCmdPipelineBarrier(cb,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,1,&barrier,0,NULL);vkCmdWriteTimestamp(cb,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,query,1);OK(vkEndCommandBuffer(cb));
 VkFence fence;VkFenceCreateInfo fc={.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};OK(vkCreateFence(device,&fc,NULL,&fence));VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cb};
 VkPhysicalDeviceProperties properties;vkGetPhysicalDeviceProperties(physical,&properties);double gpu_ns=0,started=seconds();
 for(unsigned i=0;i<iterations;i++){
  *(volatile uint32_t*)mapped=i+1;assert(!mprotect(scratch,page,PROT_READ));assert(!mprotect(scratch,page,PROT_READ|PROT_WRITE));OK(vkQueueSubmit(queue,1,&submit,fence));OK(vkWaitForFences(device,1,&fence,VK_TRUE,UINT64_MAX));
  assert(*(volatile uint32_t*)mapped==i+1);for(unsigned j=0;j<1024;j++)assert(((volatile uint32_t*)mapped)[(bytes-4096)/4+j]==0x7a35b19e);
  uint64_t times[2];OK(vkGetQueryPoolResults(device,query,0,2,sizeof(times),times,8,VK_QUERY_RESULT_64_BIT));gpu_ns+=(double)(times[1]-times[0])*properties.limits.timestampPeriod;OK(vkResetFences(device,1,&fence));
 }
 double elapsed=seconds()-started;
 printf("{\"mode\":\"%s\",\"allocation_bytes\":%llu,\"buffer_bytes\":%llu,\"iterations\":%u,\"wall_ms\":%.6f,\"gpu_ms\":%.6f,\"output_ok\":true}\n",mode==0?"aligned-generic":mode==1?"tail-generic":"native-dedicated",(unsigned long long)allocation,(unsigned long long)bytes,iterations,elapsed*1000,gpu_ns/1e6);fflush(stdout);
 assert(!munmap(scratch,page));
 vkDestroyFence(device,fence,NULL);vkDestroyQueryPool(device,query,NULL);vkDestroyCommandPool(device,pool,NULL);vkUnmapMemory(device,memory);vkDestroyBuffer(device,buffer,NULL);vkFreeMemory(device,memory,NULL);
}
int main(int argc,char **argv){
 unsigned iterations=argc>1?(unsigned)strtoul(argv[1],NULL,10):100;assert(iterations);
 VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};const char *instance_extensions[]={VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};VkInstanceCreateInfo ii={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app,.enabledExtensionCount=1,.ppEnabledExtensionNames=instance_extensions};VkInstance instance;OK(vkCreateInstance(&ii,NULL,&instance));VkPhysicalDevice physical;uint32_t count=1;OK(vkEnumeratePhysicalDevices(instance,&count,&physical));
 uint32_t n=0;vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,NULL);VkQueueFamilyProperties *families=calloc(n,sizeof(*families));assert(families);vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,families);uint32_t family=0;while(family<n && (!(families[family].queueFlags&VK_QUEUE_COMPUTE_BIT)||!families[family].timestampValidBits))family++;assert(family<n);free(families);
 OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&n,NULL));VkExtensionProperties *extensions=calloc(n,sizeof(*extensions));assert(extensions);OK(vkEnumerateDeviceExtensionProperties(physical,NULL,&n,extensions));const char *names[]={"VK_KHR_portability_subset"};unsigned nn=0;for(unsigned i=0;i<n;i++)if(!strcmp(extensions[i].extensionName,names[0]))nn=1;free(extensions);
 float priority=1;VkDeviceQueueCreateInfo qi={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.queueCreateInfoCount=1,.pQueueCreateInfos=&qi,.enabledExtensionCount=nn,.ppEnabledExtensionNames=names};VkDevice device;OK(vkCreateDevice(physical,&di,NULL,&device));VkQueue queue;vkGetDeviceQueue(device,family,0,&queue);
 run(physical,device,queue,family,0,iterations);
 vkDestroyDevice(device,NULL);vkDestroyInstance(instance,NULL);return 0;
}
