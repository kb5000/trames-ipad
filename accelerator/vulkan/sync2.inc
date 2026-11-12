// SPDX-License-Identifier: GPL-3.0-or-later
// Masks and dependency boundaries stay 64-bit. Common calls use stack scratch;
// unusually large arrays spill to the same call-owned arena as render graphs.
typedef struct {RGArena spill;size_t used;uint64_t words[192];} S2Arena;
static void s2_init(S2Arena *a){a->spill=(RGArena){0};a->used=0;}
static uint64_t *s2_alloc(S2Arena *a,size_t count,size_t width){
    if(!count)return NULL;
    if(count<=(192-a->used)/width){uint64_t *p=a->words+a->used;a->used+=count*width;return p;}
    return rg_alloc(&a->spill,count,width);
}
static void s2_free(S2Arena *a){while(a->spill.blocks){RGBlock *next=a->spill.blocks->next;free(a->spill.blocks);a->spill.blocks=next;}}
static void s2_dependency(S2Arena *a,const VkDependencyInfo *d,uint64_t h[7]){
    if(!VALID(d,VK_STRUCTURE_TYPE_DEPENDENCY_INFO)){a->spill.result=VK_ERROR_FEATURE_NOT_PRESENT;return;}
    uint64_t *mem=s2_alloc(a,d->memoryBarrierCount,4),*buf=s2_alloc(a,d->bufferMemoryBarrierCount,9),*im=s2_alloc(a,d->imageMemoryBarrierCount,14);
    if(a->spill.result!=VK_SUCCESS)return;
    uint64_t header[7]={d->dependencyFlags,d->memoryBarrierCount,PTR(mem),d->bufferMemoryBarrierCount,PTR(buf),d->imageMemoryBarrierCount,PTR(im)};memcpy(h,header,sizeof(header));
    for(uint32_t i=0;i<d->memoryBarrierCount;i++){
        const VkMemoryBarrier2 *v=d->pMemoryBarriers+i;uint64_t *r=mem+i*4;
        if(!VALID(v,VK_STRUCTURE_TYPE_MEMORY_BARRIER_2)){a->spill.result=VK_ERROR_FEATURE_NOT_PRESENT;return;}
        r[0]=v->srcStageMask;r[1]=v->srcAccessMask;r[2]=v->dstStageMask;r[3]=v->dstAccessMask;
    }
    for(uint32_t i=0;i<d->bufferMemoryBarrierCount;i++){
        const VkBufferMemoryBarrier2 *v=d->pBufferMemoryBarriers+i;uint64_t *r=buf+i*9;
        if(!VALID(v,VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2)){a->spill.result=VK_ERROR_FEATURE_NOT_PRESENT;return;}
        r[0]=(uint64_t)v->buffer;r[1]=v->srcStageMask;r[2]=v->srcAccessMask;r[3]=v->dstStageMask;r[4]=v->dstAccessMask;r[5]=v->srcQueueFamilyIndex;r[6]=v->dstQueueFamilyIndex;r[7]=v->offset;r[8]=v->size;
    }
    for(uint32_t i=0;i<d->imageMemoryBarrierCount;i++){
        const VkImageMemoryBarrier2 *v=d->pImageMemoryBarriers+i;uint64_t *r=im+i*14;
        if(!VALID(v,VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2)){a->spill.result=VK_ERROR_FEATURE_NOT_PRESENT;return;}
        r[0]=(uint64_t)v->image;r[1]=v->srcStageMask;r[2]=v->srcAccessMask;r[3]=v->dstStageMask;r[4]=v->dstAccessMask;r[5]=v->oldLayout;r[6]=v->newLayout;r[7]=v->srcQueueFamilyIndex;r[8]=v->dstQueueFamilyIndex;r[9]=v->subresourceRange.aspectMask;r[10]=v->subresourceRange.baseMipLevel;r[11]=v->subresourceRange.levelCount;r[12]=v->subresourceRange.baseArrayLayer;r[13]=v->subresourceRange.layerCount;
    }
}
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2(VkCommandBuffer c,const VkDependencyInfo *d){S2Arena a;s2_init(&a);uint64_t h[7];s2_dependency(&a,d,h);VkResult r=a.spill.result;if(r==VK_SUCCESS)r=CALL(266,DEVKEY(c),KEY(c),PTR(h));s2_free(&a);checked(r);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent2(VkCommandBuffer c,VkEvent e,const VkDependencyInfo *d){S2Arena a;s2_init(&a);uint64_t h[7];s2_dependency(&a,d,h);VkResult r=a.spill.result;if(r==VK_SUCCESS)r=CALL(267,DEVKEY(c),KEY(c),(uint64_t)e,PTR(h));s2_free(&a);checked(r);}
VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent2(VkCommandBuffer c,VkEvent e,VkPipelineStageFlags2 stage){checked(CALL(268,DEVKEY(c),KEY(c),(uint64_t)e,stage));}
VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents2(VkCommandBuffer c,uint32_t count,const VkEvent *events,const VkDependencyInfo *deps){
    S2Arena a;s2_init(&a);uint64_t *h=s2_alloc(&a,count,7);
    if(a.spill.result==VK_SUCCESS)for(uint32_t i=0;i<count && a.spill.result==VK_SUCCESS;i++)s2_dependency(&a,deps+i,h+i*7);
    VkResult r=a.spill.result;if(r==VK_SUCCESS)r=CALL(269,DEVKEY(c),KEY(c),PTR(events),PTR(h),count);s2_free(&a);checked(r);
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp2(VkCommandBuffer c,VkPipelineStageFlags2 stage,VkQueryPool p,uint32_t query){checked(CALL(270,DEVKEY(c),KEY(c),(uint64_t)p,query,stage));}
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2(VkQueue q,uint32_t count,const VkSubmitInfo2 *infos,VkFence f){
    S2Arena a;s2_init(&a);uint64_t *h=s2_alloc(&a,count,7);
    for(uint32_t i=0;i<count && a.spill.result==VK_SUCCESS;i++){
        const VkSubmitInfo2 *s=infos+i;if(!VALID(s,VK_STRUCTURE_TYPE_SUBMIT_INFO_2)){a.spill.result=VK_ERROR_FEATURE_NOT_PRESENT;break;}
        uint64_t *w=s2_alloc(&a,s->waitSemaphoreInfoCount,4),*c=s2_alloc(&a,s->commandBufferInfoCount,2),*signal=s2_alloc(&a,s->signalSemaphoreInfoCount,4);if(a.spill.result!=VK_SUCCESS)break;
        uint64_t row[7]={s->flags,PTR(w),s->waitSemaphoreInfoCount,PTR(c),s->commandBufferInfoCount,PTR(signal),s->signalSemaphoreInfoCount};memcpy(h+i*7,row,sizeof(row));
        for(unsigned dir=0;dir<2;dir++){
            const VkSemaphoreSubmitInfo *v=dir?s->pSignalSemaphoreInfos:s->pWaitSemaphoreInfos;uint32_t n=dir?s->signalSemaphoreInfoCount:s->waitSemaphoreInfoCount;uint64_t *p=dir?signal:w;
            for(uint32_t j=0;j<n;j++){
                if(!VALID(v+j,VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO)){a.spill.result=VK_ERROR_FEATURE_NOT_PRESENT;break;}
                p[j*4]=(uint64_t)v[j].semaphore;p[j*4+1]=v[j].value;p[j*4+2]=v[j].stageMask;p[j*4+3]=v[j].deviceIndex;
            }
        }
        for(uint32_t j=0;j<s->commandBufferInfoCount;j++){
            const VkCommandBufferSubmitInfo *v=s->pCommandBufferInfos+j;if(!VALID(v,VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO)){a.spill.result=VK_ERROR_FEATURE_NOT_PRESENT;break;}
            c[j*2]=KEY(v->commandBuffer);c[j*2+1]=v->deviceMask;
        }
    }
    VkResult r=a.spill.result;if(r==VK_SUCCESS)r=CALL(271,DEVKEY(q),KEY(q),PTR(h),count,(uint64_t)f);s2_free(&a);return r;
}
VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2KHR(VkCommandBuffer c,const VkDependencyInfo *d){vkCmdPipelineBarrier2(c,d);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent2KHR(VkCommandBuffer c,VkEvent e,const VkDependencyInfo *d){vkCmdSetEvent2(c,e,d);}
VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent2KHR(VkCommandBuffer c,VkEvent e,VkPipelineStageFlags2 stage){vkCmdResetEvent2(c,e,stage);}
VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents2KHR(VkCommandBuffer c,uint32_t count,const VkEvent *e,const VkDependencyInfo *d){vkCmdWaitEvents2(c,count,e,d);}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp2KHR(VkCommandBuffer c,VkPipelineStageFlags2 stage,VkQueryPool p,uint32_t query){vkCmdWriteTimestamp2(c,stage,p,query);}
VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR(VkQueue q,uint32_t count,const VkSubmitInfo2 *s,VkFence f){return vkQueueSubmit2(q,count,s,f);}
