// SPDX-License-Identifier: GPL-3.0-or-later
// Host transfers finish on the original caller. Only flat records cross the ABI.
static void host_region(uint64_t out[13],const void *pointer,uint32_t row,uint32_t height,VkImageSubresourceLayers sub,VkOffset3D offset,VkExtent3D extent){
    uint64_t v[13]={PTR(pointer),row,height,sub.aspectMask,sub.mipLevel,sub.baseArrayLayer,sub.layerCount,(uint64_t)(int64_t)offset.x,(uint64_t)(int64_t)offset.y,(uint64_t)(int64_t)offset.z,extent.width,extent.height,extent.depth};memcpy(out,v,sizeof(v));
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToImageEXT(VkDevice d,const VkCopyMemoryToImageInfoEXT *info){
    if(!VALID(info,VK_STRUCTURE_TYPE_COPY_MEMORY_TO_IMAGE_INFO_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;
    for(uint32_t at=0;at<info->regionCount;){uint64_t wire[32][13];unsigned count=info->regionCount-at;if(count>32)count=32;
        for(unsigned i=0;i<count;i++){const VkMemoryToImageCopyEXT *r=info->pRegions+at+i;if(!VALID(r,VK_STRUCTURE_TYPE_MEMORY_TO_IMAGE_COPY_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;host_region(wire[i],r->pHostPointer,r->memoryRowLength,r->memoryImageHeight,r->imageSubresource,r->imageOffset,r->imageExtent);}
        VkResult result=CALL(259,KEY(d),(uint64_t)info->dstImage,info->dstImageLayout,0,info->flags,PTR(wire),count);if(result!=VK_SUCCESS)return result;at+=count;
    }return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToMemoryEXT(VkDevice d,const VkCopyImageToMemoryInfoEXT *info){
    if(!VALID(info,VK_STRUCTURE_TYPE_COPY_IMAGE_TO_MEMORY_INFO_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;
    for(uint32_t at=0;at<info->regionCount;){uint64_t wire[32][13];unsigned count=info->regionCount-at;if(count>32)count=32;
        for(unsigned i=0;i<count;i++){const VkImageToMemoryCopyEXT *r=info->pRegions+at+i;if(!VALID(r,VK_STRUCTURE_TYPE_IMAGE_TO_MEMORY_COPY_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;host_region(wire[i],r->pHostPointer,r->memoryRowLength,r->memoryImageHeight,r->imageSubresource,r->imageOffset,r->imageExtent);}
        VkResult result=CALL(260,KEY(d),(uint64_t)info->srcImage,info->srcImageLayout,0,info->flags,PTR(wire),count);if(result!=VK_SUCCESS)return result;at+=count;
    }return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToImageEXT(VkDevice d,const VkCopyImageToImageInfoEXT *info){
    if(!VALID(info,VK_STRUCTURE_TYPE_COPY_IMAGE_TO_IMAGE_INFO_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;
    for(uint32_t at=0;at<info->regionCount;){uint64_t wire[32][17];unsigned count=info->regionCount-at;if(count>32)count=32;
        for(unsigned i=0;i<count;i++){const VkImageCopy2 *r=info->pRegions+at+i;if(!VALID(r,VK_STRUCTURE_TYPE_IMAGE_COPY_2))return VK_ERROR_FEATURE_NOT_PRESENT;uint64_t v[17]={r->srcSubresource.aspectMask,r->srcSubresource.mipLevel,r->srcSubresource.baseArrayLayer,r->srcSubresource.layerCount,(uint64_t)(int64_t)r->srcOffset.x,(uint64_t)(int64_t)r->srcOffset.y,(uint64_t)(int64_t)r->srcOffset.z,r->dstSubresource.aspectMask,r->dstSubresource.mipLevel,r->dstSubresource.baseArrayLayer,r->dstSubresource.layerCount,(uint64_t)(int64_t)r->dstOffset.x,(uint64_t)(int64_t)r->dstOffset.y,(uint64_t)(int64_t)r->dstOffset.z,r->extent.width,r->extent.height,r->extent.depth};memcpy(wire[i],v,sizeof(v));}
        VkResult result=CALL(261,KEY(d),(uint64_t)info->srcImage,info->srcImageLayout,0,info->flags,(uint64_t)info->dstImage,info->dstImageLayout,PTR(wire),count);if(result!=VK_SUCCESS)return result;at+=count;
    }return VK_SUCCESS;
}
VKAPI_ATTR VkResult VKAPI_CALL vkTransitionImageLayoutEXT(VkDevice d,uint32_t count,const VkHostImageLayoutTransitionInfoEXT *infos){
    for(uint32_t at=0;at<count;){uint64_t wire[32][8];unsigned n=count-at;if(n>32)n=32;
        for(unsigned i=0;i<n;i++){const VkHostImageLayoutTransitionInfoEXT *r=infos+at+i;if(!VALID(r,VK_STRUCTURE_TYPE_HOST_IMAGE_LAYOUT_TRANSITION_INFO_EXT))return VK_ERROR_FEATURE_NOT_PRESENT;uint64_t v[8]={(uint64_t)r->image,r->oldLayout,r->newLayout,r->subresourceRange.aspectMask,r->subresourceRange.baseMipLevel,r->subresourceRange.levelCount,r->subresourceRange.baseArrayLayer,r->subresourceRange.layerCount};memcpy(wire[i],v,sizeof(v));}
        VkResult result=CALL(262,KEY(d),PTR(wire),n);if(result!=VK_SUCCESS)return result;at+=n;
    }return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout2EXT(VkDevice d,VkImage image,const VkImageSubresource2EXT *sub,VkSubresourceLayout2EXT *out){
    if(!VALID(sub,VK_STRUCTURE_TYPE_IMAGE_SUBRESOURCE_2_EXT))checked(VK_ERROR_FEATURE_NOT_PRESENT);query_output(out,VK_STRUCTURE_TYPE_SUBRESOURCE_LAYOUT_2_EXT);
    VkSubresourceHostMemcpySizeEXT *size=NULL;for(VkBaseOutStructure *n=out->pNext;n;n=n->pNext)if(n->sType==VK_STRUCTURE_TYPE_SUBRESOURCE_HOST_MEMCPY_SIZE_EXT)size=(void*)n;
    checked(CALL(263,KEY(d),(uint64_t)image,sub->imageSubresource.aspectMask,sub->imageSubresource.mipLevel,sub->imageSubresource.arrayLayer,PTR(&out->subresourceLayout),size?PTR(&size->size):0));
}
