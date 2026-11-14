// SPDX-License-Identifier: GPL-3.0-or-later
// Same native/guest test: independent clipping, clamping, shader depth and cache.
#include <vulkan/vulkan.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OK(x) do { VkResult result=(x); if(result!=VK_SUCCESS){fprintf(stderr,"%s: %d at %d\n",#x,result,__LINE__);abort();} } while(0)
#define W 8
#define H 8
static VkPhysicalDevice physical;
static VkDevice d;
struct image { VkImage image; VkImageView view; VkDeviceMemory memory; };
static unsigned memory_type(unsigned bits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties p; vkGetPhysicalDeviceMemoryProperties(physical,&p);
    for(unsigned i=0;i<p.memoryTypeCount;i++) if((bits&(1u<<i)) && (p.memoryTypes[i].propertyFlags&flags)==flags) return i;
    abort();
}
static struct image image(VkFormat format) {
    struct image x={0}; VkImageAspectFlags aspect=format==VK_FORMAT_D32_SFLOAT?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageCreateInfo ci={.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,.imageType=VK_IMAGE_TYPE_2D,.format=format,.extent={W,H,1},.mipLevels=1,.arrayLayers=1,.samples=VK_SAMPLE_COUNT_1_BIT,.usage=VK_IMAGE_USAGE_TRANSFER_SRC_BIT|(aspect==VK_IMAGE_ASPECT_DEPTH_BIT?VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)};
    OK(vkCreateImage(d,&ci,NULL,&x.image)); VkMemoryRequirements r; vkGetImageMemoryRequirements(d,x.image,&r);
    VkMemoryAllocateInfo a={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=r.size,.memoryTypeIndex=memory_type(r.memoryTypeBits,0)};
    OK(vkAllocateMemory(d,&a,NULL,&x.memory)); OK(vkBindImageMemory(d,x.image,x.memory,0));
    VkImageViewCreateInfo vi={.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,.image=x.image,.viewType=VK_IMAGE_VIEW_TYPE_2D,.format=format,.subresourceRange={aspect,0,1,0,1}};
    OK(vkCreateImageView(d,&vi,NULL,&x.view)); return x;
}
static VkShaderModule shader(const char *path) {
    FILE *f=fopen(path,"rb"); assert(f); assert(!fseek(f,0,SEEK_END)); long n=ftell(f); assert(n>0 && !(n%4)); rewind(f);
    uint32_t *code=malloc((size_t)n); assert(code && fread(code,1,(size_t)n,f)==(size_t)n); fclose(f);
    VkShaderModuleCreateInfo ci={.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,.codeSize=(size_t)n,.pCode=code}; VkShaderModule s;
    OK(vkCreateShaderModule(d,&ci,NULL,&s)); free(code); return s;
}
int main(int argc,char **argv) {
    assert(argc==5);
    VkApplicationInfo app={.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,.apiVersion=VK_API_VERSION_1_2};
    VkInstanceCreateInfo ii={.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,.pApplicationInfo=&app}; VkInstance instance; OK(vkCreateInstance(&ii,NULL,&instance));
    unsigned n=1; OK(vkEnumeratePhysicalDevices(instance,&n,&physical)); assert(n==1);
    VkPhysicalDeviceDepthClipEnableFeaturesEXT clip={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT};
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT eds={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,.pNext=&clip};
    VkPhysicalDeviceFeatures2 features={.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,.pNext=&eds}; vkGetPhysicalDeviceFeatures2(physical,&features);
    assert(clip.depthClipEnable && features.features.depthClamp && eds.extendedDynamicState3DepthClipEnable && eds.extendedDynamicState3DepthClampEnable);
    printf("queried logicOp=%u; depthClipEnable=%u; dynamicDepthClipEnable=%u\n",features.features.logicOp,clip.depthClipEnable,eds.extendedDynamicState3DepthClipEnable);
    vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,NULL); VkQueueFamilyProperties *q=calloc(n,sizeof(*q)); assert(q); vkGetPhysicalDeviceQueueFamilyProperties(physical,&n,q);
    unsigned family=0; while(family<n && !(q[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))family++; assert(family<n); free(q);
    float priority=1; VkDeviceQueueCreateInfo qi={.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,.queueFamilyIndex=family,.queueCount=1,.pQueuePriorities=&priority};
    const char *extensions[]={VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME};
    memset(&eds,0,sizeof(eds)); eds.sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT; eds.pNext=&clip;
    eds.extendedDynamicState3DepthClampEnable=eds.extendedDynamicState3DepthClipEnable=VK_TRUE;
    VkPhysicalDeviceFeatures enabled={.depthClamp=VK_TRUE};
    VkDeviceCreateInfo di={.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,.pNext=&eds,.queueCreateInfoCount=1,.pQueueCreateInfos=&qi,.enabledExtensionCount=2,.ppEnabledExtensionNames=extensions,.pEnabledFeatures=&enabled};
    OK(vkCreateDevice(physical,&di,NULL,&d)); VkQueue queue; vkGetDeviceQueue(d,family,0,&queue);
    PFN_vkCmdSetDepthClipEnableEXT set_clip=(PFN_vkCmdSetDepthClipEnableEXT)vkGetDeviceProcAddr(d,"vkCmdSetDepthClipEnableEXT");
    PFN_vkCmdSetDepthClampEnableEXT set_clamp=(PFN_vkCmdSetDepthClampEnableEXT)vkGetDeviceProcAddr(d,"vkCmdSetDepthClampEnableEXT"); assert(set_clip && set_clamp);
    struct image colors=image(VK_FORMAT_R8G8B8A8_UNORM),depth=image(VK_FORMAT_D32_SFLOAT);
    VkAttachmentDescription attachments[2]={
        {.format=VK_FORMAT_R8G8B8A8_UNORM,.samples=VK_SAMPLE_COUNT_1_BIT,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_STORE,.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
        {.format=VK_FORMAT_D32_SFLOAT,.samples=VK_SAMPLE_COUNT_1_BIT,.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,.storeOp=VK_ATTACHMENT_STORE_OP_STORE,.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED,.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}};
    VkAttachmentReference cr={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},dr={1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub={.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS,.colorAttachmentCount=1,.pColorAttachments=&cr,.pDepthStencilAttachment=&dr};
    VkSubpassDependency dep={.srcSubpass=0,.dstSubpass=VK_SUBPASS_EXTERNAL,.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT,.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT};
    VkRenderPassCreateInfo ri={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,.attachmentCount=2,.pAttachments=attachments,.subpassCount=1,.pSubpasses=&sub,.dependencyCount=1,.pDependencies=&dep}; VkRenderPass pass; OK(vkCreateRenderPass(d,&ri,NULL,&pass));
    VkImageView views[]={colors.view,depth.view}; VkFramebufferCreateInfo fi={.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,.renderPass=pass,.attachmentCount=2,.pAttachments=views,.width=W,.height=H,.layers=1}; VkFramebuffer framebuffer; OK(vkCreateFramebuffer(d,&fi,NULL,&framebuffer));
    VkPushConstantRange push={VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,0,8}; VkPipelineLayoutCreateInfo li={.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,.pushConstantRangeCount=1,.pPushConstantRanges=&push}; VkPipelineLayout layout; OK(vkCreatePipelineLayout(d,&li,NULL,&layout));
    VkBufferCreateInfo bi={.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,.size=W*H*8,.usage=VK_BUFFER_USAGE_TRANSFER_DST_BIT}; VkBuffer buffer; OK(vkCreateBuffer(d,&bi,NULL,&buffer));
    VkMemoryRequirements br; vkGetBufferMemoryRequirements(d,buffer,&br); VkMemoryAllocateInfo ma={.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,.allocationSize=br.size,.memoryTypeIndex=memory_type(br.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)}; VkDeviceMemory memory; OK(vkAllocateMemory(d,&ma,NULL,&memory)); OK(vkBindBufferMemory(d,buffer,memory,0)); void *mapped; OK(vkMapMemory(d,memory,0,VK_WHOLE_SIZE,0,&mapped));
    VkCommandPoolCreateInfo pi={.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,.queueFamilyIndex=family}; VkCommandPool pool; OK(vkCreateCommandPool(d,&pi,NULL,&pool));
    VkCommandBufferAllocateInfo ca={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,.commandBufferCount=1}; VkCommandBuffer cmd; OK(vkAllocateCommandBuffers(d,&ca,&cmd));
    VkShaderModule vertex=shader(argv[1]),fragments[]={shader(argv[2]),shader(argv[3]),shader(argv[4])};
    VkPipelineCacheCreateInfo pci={.sType=VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO}; VkPipelineCache cache; OK(vkCreatePipelineCache(d,&pci,NULL,&cache)); unsigned tested=0;
    for(unsigned reload=0;reload<2;reload++) {
        if(reload){size_t size=0; OK(vkGetPipelineCacheData(d,cache,&size,NULL)); void *data=malloc(size); assert(data); OK(vkGetPipelineCacheData(d,cache,&size,data)); vkDestroyPipelineCache(d,cache,NULL); pci.initialDataSize=size; pci.pInitialData=data; OK(vkCreatePipelineCache(d,&pci,NULL,&cache)); free(data);}
        for(unsigned mode=0;mode<3;mode++)for(unsigned dynamic=0;dynamic<2;dynamic++)for(unsigned clamp=0;clamp<2;clamp++)for(unsigned clip_mode=0;clip_mode<3;clip_mode++) {
            // clip_mode 2 deliberately omits the extension state: core default.
            unsigned clipping=clip_mode==2?!clamp:clip_mode;
            VkPipelineRasterizationDepthClipStateCreateInfoEXT dc={.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,.depthClipEnable=clipping};
            VkPipelineRasterizationStateCreateInfo rs={.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,.pNext=clip_mode==2?NULL:&dc,.depthClampEnable=clamp,.lineWidth=1};
            VkPipelineShaderStageCreateInfo stages[2]={{.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_VERTEX_BIT,.module=vertex,.pName="main"},{.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,.stage=VK_SHADER_STAGE_FRAGMENT_BIT,.module=fragments[mode],.pName="main"}};
            VkPipelineVertexInputStateCreateInfo vi={.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO}; VkPipelineInputAssemblyStateCreateInfo ia={.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
            VkPipelineViewportStateCreateInfo vp={.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,.viewportCount=1,.scissorCount=1};
            VkPipelineMultisampleStateCreateInfo ms={.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT};
            VkPipelineDepthStencilStateCreateInfo ds={.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,.depthTestEnable=VK_TRUE,.depthWriteEnable=VK_TRUE,.depthCompareOp=VK_COMPARE_OP_ALWAYS};
            VkPipelineColorBlendAttachmentState ba={.colorWriteMask=15}; VkPipelineColorBlendStateCreateInfo bs={.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,.attachmentCount=1,.pAttachments=&ba};
            VkDynamicState states[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT,VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT};
            VkPipelineDynamicStateCreateInfo dy={.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,.dynamicStateCount=dynamic?4:2,.pDynamicStates=states};
            VkGraphicsPipelineCreateInfo gi={.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,.stageCount=2,.pStages=stages,.pVertexInputState=&vi,.pInputAssemblyState=&ia,.pViewportState=&vp,.pRasterizationState=&rs,.pMultisampleState=&ms,.pDepthStencilState=&ds,.pColorBlendState=&bs,.pDynamicState=&dy,.layout=layout,.renderPass=pass}; VkPipeline pipeline; OK(vkCreateGraphicsPipelines(d,cache,1,&gi,NULL,&pipeline));
            for(unsigned reversed=0;reversed<2;reversed++)for(unsigned zcase=0;zcase<3;zcase++) {
                float z[]={-.25f,.5f,1.25f}; float params[]={z[zcase],zcase==0?.1f:.9f}; float min=reversed?.75f:.25f,max=reversed?.25f:.75f;
                OK(vkResetCommandBuffer(cmd,0)); VkCommandBufferBeginInfo cb={.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; OK(vkBeginCommandBuffer(cmd,&cb));
                VkClearValue clear[2]={{.color={.float32={0,0,0,1}}}, {.depthStencil={.94f,0}}}; VkRenderPassBeginInfo begin={.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,.renderPass=pass,.framebuffer=framebuffer,.renderArea={{0,0},{W,H}},.clearValueCount=2,.pClearValues=clear};
                vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE); vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
                VkViewport viewport={0,0,W,H,min,max}; VkRect2D scissor={{0,0},{W,H}}; vkCmdSetViewport(cmd,0,1,&viewport); vkCmdSetScissor(cmd,0,1,&scissor);
                if(dynamic){set_clamp(cmd,clamp);set_clip(cmd,clipping);}
                vkCmdPushConstants(cmd,layout,push.stageFlags,0,8,params); vkCmdDraw(cmd,3,1,0,0); vkCmdEndRenderPass(cmd);
                VkBufferImageCopy copy={.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1},.imageExtent={W,H,1}}; vkCmdCopyImageToBuffer(cmd,colors.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
                copy.bufferOffset=W*H*4;copy.imageSubresource.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;vkCmdCopyImageToBuffer(cmd,depth.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,buffer,1,&copy);
                VkMemoryBarrier barrier={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT,.dstAccessMask=VK_ACCESS_HOST_READ_BIT}; vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,NULL,0,NULL);
                OK(vkEndCommandBuffer(cmd)); VkSubmitInfo submit={.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,.commandBufferCount=1,.pCommandBuffers=&cmd}; OK(vkQueueSubmit(queue,1,&submit,VK_NULL_HANDLE)); OK(vkQueueWaitIdle(queue));
                int discarded=clipping && zcase!=1; float want=mode==1?params[1]:min+(max-min)*params[0]; if(clamp)want=fminf(.75f,fmaxf(.25f,want));if(discarded)want=.94f;
                for(unsigned pixel=0;pixel<W*H;pixel++) {
                    const unsigned char *color=(const unsigned char *)mapped+pixel*4; const float *depths=(const float *)((const unsigned char *)mapped+W*H*4);
                    if(color[0]!=(discarded?0:255) || color[1] || color[2] || fabsf(depths[pixel]-want)>.00001f){fprintf(stderr,"FAIL reload=%u mode=%u dynamic=%u clamp=%u clip=%u reversed=%u z=%g pixel=%u color=%u depth=%g want=%g\n",reload,mode,dynamic,clamp,clip_mode,reversed,params[0],pixel,color[0],depths[pixel],want);abort();}
                }
                tested++;
            }
            vkDestroyPipeline(d,pipeline,NULL);
        }
    }
    printf("vulkan-depth-clip-ok: %u renders; color/depth, static/dynamic, default clip, reversed viewport, explicit depth, early tests, cache reload\n",tested);
    vkDestroyPipelineCache(d,cache,NULL);vkDestroyShaderModule(d,vertex,NULL);for(unsigned i=0;i<3;i++)vkDestroyShaderModule(d,fragments[i],NULL);
    vkDestroyCommandPool(d,pool,NULL);vkUnmapMemory(d,memory);vkDestroyBuffer(d,buffer,NULL);vkFreeMemory(d,memory,NULL);vkDestroyPipelineLayout(d,layout,NULL);vkDestroyFramebuffer(d,framebuffer,NULL);vkDestroyRenderPass(d,pass,NULL);
    struct image images[]={colors,depth};for(unsigned i=0;i<2;i++){vkDestroyImageView(d,images[i].view,NULL);vkDestroyImage(d,images[i].image,NULL);vkFreeMemory(d,images[i].memory,NULL);}vkDestroyDevice(d,NULL);vkDestroyInstance(instance,NULL);return 0;
}
