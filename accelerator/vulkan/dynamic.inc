// SPDX-License-Identifier: GPL-3.0-or-later
// Dynamic rendering retains the original attachment views. Stack wire records
// are copied synchronously; neither Vulkan structs nor guest pointers escape.
static void rendering_attachment(uint64_t out[9],const VkRenderingAttachmentInfo *info) {
    out[0]=(uint64_t)info->imageView;out[1]=info->imageLayout;out[2]=info->resolveMode;out[3]=(uint64_t)info->resolveImageView;out[4]=info->resolveImageLayout;out[5]=info->loadOp;out[6]=info->storeOp;
    memcpy(out+7,&info->clearValue,16);
}
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRendering(VkCommandBuffer c,const VkRenderingInfo *info) {
    if(!info || info->sType!=VK_STRUCTURE_TYPE_RENDERING_INFO || info->colorAttachmentCount>8)checked(VK_ERROR_FEATURE_NOT_PRESENT);
    uint64_t attachments[10][9],areas[4];
    for(uint32_t i=0;i<info->colorAttachmentCount;i++)rendering_attachment(attachments[i],info->pColorAttachments+i);
    if(info->pDepthAttachment)rendering_attachment(attachments[8],info->pDepthAttachment);
    if(info->pStencilAttachment)rendering_attachment(attachments[9],info->pStencilAttachment);
    uint64_t h[11]={info->flags,(uint64_t)(int64_t)info->renderArea.offset.x,(uint64_t)(int64_t)info->renderArea.offset.y,info->renderArea.extent.width,info->renderArea.extent.height,info->layerCount,info->viewMask,info->colorAttachmentCount,PTR(attachments),info->pDepthAttachment?PTR(attachments[8]):0,info->pStencilAttachment?PTR(attachments[9]):0};
    const VkDeviceGroupRenderPassBeginInfo *g=find_next(info->pNext,VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO);
    if(g && (g->deviceMask!=1 || g->deviceRenderAreaCount>1))checked(VK_ERROR_FEATURE_NOT_PRESENT);
    if(g && g->deviceRenderAreaCount){areas[0]=(uint64_t)(int64_t)g->pDeviceRenderAreas[0].offset.x;areas[1]=(uint64_t)(int64_t)g->pDeviceRenderAreas[0].offset.y;areas[2]=g->pDeviceRenderAreas[0].extent.width;areas[3]=g->pDeviceRenderAreas[0].extent.height;}
    checked(CALL(253,DEVKEY(c),KEY(c),PTR(h),g?g->deviceMask:0,PTR(areas),g?g->deviceRenderAreaCount:0));
}
VKAPI_ATTR void VKAPI_CALL vkCmdEndRendering(VkCommandBuffer c) {checked(CALL(254,DEVKEY(c),KEY(c)));}
VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR(VkCommandBuffer c,const VkRenderingInfo *info) {vkCmdBeginRendering(c,info);}
VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR(VkCommandBuffer c) {vkCmdEndRendering(c);}
