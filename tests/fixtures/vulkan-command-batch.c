// SPDX-License-Identifier: GPL-3.0-or-later
// Real ICD -> recording transport. Caller-owned inputs are changed immediately
// after each public call; an eventual batch must still own their old bytes.
#define _GNU_SOURCE
#include <assert.h>
#ifdef __APPLE__
#define MFD_CLOEXEC 1
static int memfd_create(const char *name,unsigned flags){(void)name;(void)flags;assert(!"unused Wayland path");return -1;}
#endif
#include "../../accelerator/vulkan/icd.c"
static unsigned native_calls,applied,expected_count,batches;
static uint64_t expected[1024][3];
static const unsigned char *packet;
static size_t packet_size;
static int payload_case,fail_batch;
static unsigned custom_live;
static int deny_scratch;
static void *custom_allocate(void *user,size_t size,size_t alignment,VkSystemAllocationScope scope){
    (void)user;assert(alignment<=16);
    if(size==sizeof(CommandBatch)){assert(scope==VK_SYSTEM_ALLOCATION_SCOPE_COMMAND);if(deny_scratch)return NULL;}
    void *p=malloc(size);if(p)custom_live++;return p;
}
static void custom_free(void *user,void *p){(void)user;if(p){assert(custom_live);custom_live--;free(p);}}
unsigned long getauxval(unsigned long key){return key==0x54520008UL?0x10006UL:0;}
static void expect(unsigned op,unsigned argument,uint64_t value){expected[expected_count][0]=op;expected[expected_count][1]=argument;expected[expected_count++][2]=value;}
static const void *bytes(uint64_t at,size_t length){
    if(!packet)return (const void *)(uintptr_t)at;
    assert(at>=8 && at<=packet_size && length<=packet_size-at);return packet+at;
}
static void verify(unsigned op,const uint64_t *a){
    assert(applied<expected_count && op==expected[applied][0]);
    assert(a[expected[applied][1]]==expected[applied][2]);applied++;
    if(!payload_case)return;
    switch(op){
    case 47:assert(!memcmp(bytes(a[5],8),"snapshot",8));break;
    case 75:{const float *v=bytes(a[2],24);assert(v[0]==2 && v[2]==640 && v[4]==0 && v[5]==1);break;}
    case 42:{const uint64_t *sets=bytes(a[4],16);const uint32_t *offsets=bytes(a[7],8);assert(sets[0]==11 && sets[1]==22 && offsets[0]==256 && offsets[1]==512);break;}
    case 213:{const unsigned char *v=bytes(a[5],a[4]);for(unsigned i=0;i<a[4];i++)assert(v[i]==(unsigned char)(i*31));break;}
    case 266:case 267:{
        const uint64_t *h=bytes(a[op==266?2:3],56);assert(h[1]==1 && h[3]==1 && h[5]==1);
        const uint64_t *m=bytes(h[2],32),*b=bytes(h[4],72),*im=bytes(h[6],112);
        assert(m[0]==(1UL<<35) && m[3]==8 && b[0]==42 && b[8]==256 && im[0]==43 && im[13]==2);break;
    }
    case 269:{
        assert(a[4]==2);const uint64_t *events=bytes(a[2],16),*h=bytes(a[3],112);assert(events[0]==5 && events[1]==6);
        for(unsigned i=0;i<2;i++){const uint64_t *m=bytes(h[i*7+2],32);assert(h[i*7+1]==1 && m[0]==(1UL<<35));}break;
    }
    case 253:{
        const uint64_t *h=bytes(a[2],88);assert(h[7]==1 && h[3]==640);
        const uint64_t *color=bytes(h[8],72),*depth=bytes(h[9],72);assert(color[0]==91 && depth[0]==92);
        const float *v=(const void *)(color+7);assert(v[0]==.25f && v[3]==1);break;
    }
    case 227:{
        const uint64_t *h=bytes(a[2],80),*events=bytes(h[1],16),*m=bytes(h[5],16),*b=bytes(h[7],56),*im=bytes(h[9],80);
        assert(h[0]==2 && events[1]==6 && m[0]==VK_ACCESS_TRANSFER_WRITE_BIT && b[0]==42 && im[0]==43);break;
    }
    case 255:
        if(a[2]==VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE){const uint64_t *v=bytes(a[5],16),*s=bytes(a[8],16);assert(v[1]==88 && s[0]==16);}
        else if(a[2]==VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT){const float *p=bytes(a[7],16);assert(p[0]==.25f && p[3]==.75f);}break;
    case 45:{const uint64_t *v=bytes(a[4],24);assert(v[0]==8 && v[1]==16 && v[2]==32);break;}
    default:break;
    }
}
struct result trames_vulkan_call(uint64_t op,const uint64_t *a,size_t size){
    assert(size==128);native_calls++;
    if(op==COMMAND_BATCH_OPERATION){
        batches++;assert(a[0]==7 && a[3] && a[3]<=64 && a[5]<=COMMAND_BATCH_BYTES);
        if(fail_batch)return (struct result){(uint64_t)(int64_t)VK_ERROR_DEVICE_LOST,0};
        packet=(const void *)(uintptr_t)a[4];packet_size=a[5];
        const uint64_t *r=(const void *)(uintptr_t)a[2];
        for(unsigned i=0;i<a[3];i++){assert(r[i*17+1]==a[0] && r[i*17+2]==a[1]);verify((unsigned)r[i*17],r+i*17+1);}
        packet=NULL;packet_size=0;
    }else verify((unsigned)op,a);
    return (struct result){0,0};
}
static void reset(void){assert(applied==expected_count);native_calls=applied=expected_count=batches=0;payload_case=fail_batch=0;}
static void init(Dispatch *d,Dispatch *c,int fast){memset(d,0,sizeof(*d));memset(c,0,sizeof(*c));d->key=7;d->device=d;d->capabilities[0]=fast?COMMAND_BATCH_CAPABILITY:0;c->key=8;c->device=d;pthread_mutex_init(&d->lock,NULL);}
static void finish(Dispatch *d,Dispatch *c){command_batch_free(c);pthread_mutex_destroy(&d->lock);assert(applied==expected_count);}
static void draws(int fast){
    reset();Dispatch d,c;init(&d,&c,fast);
    for(unsigned i=0;i<200;i++){
        expect(73,2,i+1);vkCmdDraw((void *)&c,i+1,1,0,0);
        assert(native_calls==(fast?(i+1)/64:i+1));
    }
    expect(39,1,8);assert(vkEndCommandBuffer((void *)&c)==VK_SUCCESS);
    assert(native_calls==(fast?5:201));finish(&d,&c);
}
static void payloads(int fast){
    reset();payload_case=1;Dispatch d,c;init(&d,&c,fast);VkCommandBuffer cb=(void *)&c;
    char push[9]="snapshot";expect(47,2,3);vkCmdPushConstants(cb,(VkPipelineLayout)3,VK_SHADER_STAGE_VERTEX_BIT,0,8,push);memset(push,0,sizeof(push));
    VkViewport viewport={2,4,640,480,0,1};expect(75,1,8);vkCmdSetViewport(cb,0,1,&viewport);memset(&viewport,0,sizeof(viewport));
    VkDescriptorSet sets[2]={(VkDescriptorSet)11,(VkDescriptorSet)22};uint32_t offsets[2]={256,512};expect(42,2,3);vkCmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,(VkPipelineLayout)3,0,2,sets,2,offsets);memset(sets,0,sizeof(sets));memset(offsets,0,sizeof(offsets));
    VkBufferCopy2 region={.sType=VK_STRUCTURE_TYPE_BUFFER_COPY_2,.srcOffset=8,.dstOffset=16,.size=32};VkCopyBufferInfo2 copy={.sType=VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,.srcBuffer=(VkBuffer)11,.dstBuffer=(VkBuffer)22,.regionCount=1,.pRegions=&region};expect(45,2,11);vkCmdCopyBuffer2(cb,&copy);memset(&region,0,sizeof(region));
    VkMemoryBarrier2 m={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,.srcStageMask=1UL<<35,.dstAccessMask=8};
    VkBufferMemoryBarrier2 b={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,.buffer=(VkBuffer)42,.size=256};
    VkImageMemoryBarrier2 im={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,.image=(VkImage)43,.subresourceRange={.layerCount=2}};
    VkDependencyInfo dep={.sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO,.memoryBarrierCount=1,.pMemoryBarriers=&m,.bufferMemoryBarrierCount=1,.pBufferMemoryBarriers=&b,.imageMemoryBarrierCount=1,.pImageMemoryBarriers=&im};
    expect(266,1,8);vkCmdPipelineBarrier2(cb,&dep);expect(267,2,9);vkCmdSetEvent2(cb,(VkEvent)9,&dep);
    VkEvent events[2]={(VkEvent)5,(VkEvent)6};VkDependencyInfo deps[2]={dep,dep};expect(269,4,2);vkCmdWaitEvents2(cb,2,events,deps);
    memset(&m,0,sizeof(m));memset(&b,0,sizeof(b));memset(&im,0,sizeof(im));
    VkRenderingAttachmentInfo color={.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,.imageView=(VkImageView)91,.clearValue.color.float32={.25f,.5f,.75f,1}};
    VkRenderingAttachmentInfo depth={.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,.imageView=(VkImageView)92};
    VkRenderingInfo rendering={.sType=VK_STRUCTURE_TYPE_RENDERING_INFO,.renderArea.extent={640,480},.layerCount=1,.colorAttachmentCount=1,.pColorAttachments=&color,.pDepthAttachment=&depth};
    expect(253,1,8);vkCmdBeginRendering(cb,&rendering);memset(&color,0,sizeof(color));memset(&depth,0,sizeof(depth));expect(254,1,8);vkCmdEndRendering(cb);
    VkMemoryBarrier mem={.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER,.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT};VkBufferMemoryBarrier buf={.sType=VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,.buffer=(VkBuffer)42};VkImageMemoryBarrier image={.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,.image=(VkImage)43};
    expect(227,1,8);vkCmdWaitEvents(cb,2,events,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,1,&mem,1,&buf,1,&image);memset(events,0,sizeof(events));memset(&mem,0,sizeof(mem));memset(&buf,0,sizeof(buf));memset(&image,0,sizeof(image));
    VkBuffer buffers[2]={(VkBuffer)77,(VkBuffer)88};VkDeviceSize off[2]={0,8},sizes[2]={128,256},strides[2]={16,32};expect(255,2,VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE);vkCmdBindVertexBuffers2(cb,0,2,buffers,off,sizes,strides);memset(buffers,0,sizeof(buffers));memset(strides,0,sizeof(strides));
    VkSampleLocationEXT points[2]={{.25f,.25f},{.75f,.75f}};VkSampleLocationsInfoEXT samples={.sType=VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT,.sampleLocationsPerPixel=VK_SAMPLE_COUNT_2_BIT,.sampleLocationGridSize={1,1},.sampleLocationsCount=2,.pSampleLocations=points};expect(255,2,VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT);vkCmdSetSampleLocationsEXT(cb,&samples);memset(points,0,sizeof(points));
    expect(39,1,8);assert(vkEndCommandBuffer(cb)==VK_SUCCESS);assert(batches==(fast?1:0));finish(&d,&c);
}
static void boundaries(void){
    reset();payload_case=1;Dispatch d,c;init(&d,&c,1);VkCommandBuffer cb=(void *)&c;
    unsigned char data[32768];for(unsigned i=0;i<sizeof(data);i++)data[i]=(unsigned char)(i*31);
    expect(213,2,19);vkCmdUpdateBuffer(cb,(VkBuffer)19,0,8192,data);
    expect(213,2,20);vkCmdUpdateBuffer(cb,(VkBuffer)20,0,8192,data);assert(native_calls==1);
    expect(213,2,21);vkCmdUpdateBuffer(cb,(VkBuffer)21,0,sizeof(data),data);assert(native_calls==3);
    memset(data,0,sizeof(data));expect(73,2,5);vkCmdDraw(cb,5,1,0,0);expect(39,1,8);assert(vkEndCommandBuffer(cb)==0);assert(native_calls==5);finish(&d,&c);
}
static void *draw_on_peer(void *p){vkCmdDraw(p,17,1,0,0);return NULL;}
static void migration(void){
    reset();Dispatch d,c;init(&d,&c,1);pthread_t thread;
    expect(73,2,17);assert(!pthread_create(&thread,NULL,draw_on_peer,&c));pthread_join(thread,NULL);assert(!native_calls);
    expect(73,2,18);vkCmdDraw((void *)&c,18,1,0,0);expect(39,1,8);assert(vkEndCommandBuffer((void *)&c)==0);assert(native_calls==2);finish(&d,&c);
}
static void pools(void){
    reset();Dispatch d,unused;init(&d,&unused,1);
    Dispatch *a=dispatch_with_allocator(8,&d,NULL,VK_SYSTEM_ALLOCATION_SCOPE_OBJECT),*b=dispatch_with_allocator(9,&d,NULL,VK_SYSTEM_ALLOCATION_SCOPE_OBJECT),*other=dispatch_with_allocator(10,&d,NULL,VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    a->parent=b->parent=99;other->parent=100;d.commands=a;a->next=b;b->next=other;
    expect(73,2,1);vkCmdDraw((void *)a,1,1,0,0);expect(73,2,2);vkCmdDraw((void *)b,2,1,0,0);
    // The unrelated buffer is kept pending and is not touched by pool reset.
    vkCmdDraw((void *)other,3,1,0,0);expect(53,1,99);assert(vkResetCommandPool((void *)&d,(VkCommandPool)99,0)==0);assert(native_calls==3 && other->batch->count==1);
    expect(37,2,8);VkCommandBuffer cb=(void *)a;vkFreeCommandBuffers((void *)&d,(VkCommandPool)99,1,&cb);
    expect(35,1,99);vkDestroyCommandPool((void *)&d,(VkCommandPool)99,NULL);
    expect(73,2,3);expect(35,1,100);vkDestroyCommandPool((void *)&d,(VkCommandPool)100,NULL);assert(!d.commands);finish(&d,&unused);
}
static void failed_publication(void){
    reset();Dispatch d,c;init(&d,&c,1);vkCmdDraw((void *)&c,1,1,0,0);fail_batch=1;
    assert(vkEndCommandBuffer((void *)&c)==VK_ERROR_DEVICE_LOST);assert(native_calls==1 && c.batch->count==0);
    assert(command_batch_flush(&c)==0 && native_calls==1);finish(&d,&c);
}
static void allocators(void){
    for(deny_scratch=0;deny_scratch<2;deny_scratch++){
        reset();Dispatch d,unused;init(&d,&unused,1);VkAllocationCallbacks hooks={.pfnAllocation=custom_allocate,.pfnFree=custom_free};
        Dispatch *c=dispatch_with_allocator(8,&d,&hooks,VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);assert(c);
        for(unsigned i=0;i<3;i++){expect(73,2,i+1);vkCmdDraw((void *)c,i+1,1,0,0);}
        expect(39,1,8);assert(vkEndCommandBuffer((void *)c)==0);assert(native_calls==(deny_scratch?4:2));free_dispatch(c);assert(!custom_live);finish(&d,&unused);
    }
}
int main(void){draws(0);draws(1);payloads(0);payloads(1);boundaries();migration();pools();failed_publication();allocators();puts("command batches: 200 draws 200 -> 4 publications; owned nested payloads, arena/full/oversized boundaries, thread handoff, pool isolation, custom allocator/OOM fallback and no failed replay passed");}
