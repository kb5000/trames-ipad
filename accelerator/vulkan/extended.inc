// SPDX-License-Identifier: GPL-3.0-or-later
// Original dynamic state commands. No state replay or pipeline recompilation.
static int extended_dynamic(VkDynamicState state) {
    switch(state){
    case VK_DYNAMIC_STATE_CULL_MODE:case VK_DYNAMIC_STATE_FRONT_FACE:case VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY:
    case VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT:case VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT:case VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE:
    case VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE:case VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE:case VK_DYNAMIC_STATE_DEPTH_COMPARE_OP:
    case VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE:case VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE:case VK_DYNAMIC_STATE_STENCIL_OP:
    case VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE:case VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE:case VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE:
    case VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT:case VK_DYNAMIC_STATE_LOGIC_OP_EXT:
    case VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT:case VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT:case VK_DYNAMIC_STATE_POLYGON_MODE_EXT:case VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT:case VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT:case VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT:case VK_DYNAMIC_STATE_LINE_STIPPLE_EXT:return 1;
    default:return 0;}
}
static int pipeline_dynamic(const VkGraphicsPipelineCreateInfo *info,VkDynamicState state) {
    const VkPipelineDynamicStateCreateInfo *d=info->pDynamicState;
    if(d)for(uint32_t i=0;i<d->dynamicStateCount;i++)if(d->pDynamicStates[i]==state)return 1;
    return 0;
}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCullModeEXT(VkCommandBuffer c,VkCullModeFlags value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_CULL_MODE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetFrontFaceEXT(VkCommandBuffer c,VkFrontFace value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_FRONT_FACE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer c,VkPrimitiveTopology value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthWriteEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthCompareOpEXT(VkCommandBuffer c,VkCompareOp value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBoundsTestEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilTestEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizerDiscardEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBiasEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveRestartEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPatchControlPointsEXT(VkCommandBuffer c,uint32_t value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLogicOpEXT(VkCommandBuffer c,VkLogicOp value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_LOGIC_OP_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClampEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_CLAMP_ENABLE_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPolygonModeEXT(VkCommandBuffer c,VkPolygonMode value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_POLYGON_MODE_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCountEXT(VkCommandBuffer c,uint32_t count,const VkViewport *v){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,count,PTR(v)));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCountEXT(VkCommandBuffer c,uint32_t count,const VkRect2D *v){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,count,PTR(v)));}
VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2EXT(VkCommandBuffer c,uint32_t first,uint32_t count,const VkBuffer *buffers,const VkDeviceSize *offsets,const VkDeviceSize *sizes,const VkDeviceSize *strides){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,first,count,PTR(buffers),PTR(offsets),PTR(sizes),PTR(strides)));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilOpEXT(VkCommandBuffer c,VkStencilFaceFlags faces,VkStencilOp fail,VkStencilOp pass,VkStencilOp depthFail,VkCompareOp compare){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_STENCIL_OP,faces,fail,pass,depthFail,compare));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetCullMode(VkCommandBuffer c,VkCullModeFlags value){vkCmdSetCullModeEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetFrontFace(VkCommandBuffer c,VkFrontFace value){vkCmdSetFrontFaceEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveTopology(VkCommandBuffer c,VkPrimitiveTopology value){vkCmdSetPrimitiveTopologyEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetDepthTestEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthWriteEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetDepthWriteEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthCompareOp(VkCommandBuffer c,VkCompareOp value){vkCmdSetDepthCompareOpEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBoundsTestEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetDepthBoundsTestEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilTestEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetStencilTestEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizerDiscardEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetRasterizerDiscardEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBiasEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetDepthBiasEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveRestartEnable(VkCommandBuffer c,VkBool32 value){vkCmdSetPrimitiveRestartEnableEXT(c,value);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCount(VkCommandBuffer c,uint32_t count,const VkViewport *v){vkCmdSetViewportWithCountEXT(c,count,v);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCount(VkCommandBuffer c,uint32_t count,const VkRect2D *v){vkCmdSetScissorWithCountEXT(c,count,v);}
VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2(VkCommandBuffer c,uint32_t first,uint32_t count,const VkBuffer *buffers,const VkDeviceSize *offsets,const VkDeviceSize *sizes,const VkDeviceSize *strides){vkCmdBindVertexBuffers2EXT(c,first,count,buffers,offsets,sizes,strides);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilOp(VkCommandBuffer c,VkStencilFaceFlags faces,VkStencilOp fail,VkStencilOp pass,VkStencilOp depthFail,VkCompareOp compare){vkCmdSetStencilOpEXT(c,faces,fail,pass,depthFail,compare);}
VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_ENABLE_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineRasterizationModeEXT(VkCommandBuffer c,VkLineRasterizationModeEXT value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_LINE_RASTERIZATION_MODE_EXT,value));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEXT(VkCommandBuffer c,const VkSampleLocationsInfoEXT *v){if(!VALID(v,VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT))checked(VK_ERROR_FEATURE_NOT_PRESENT);checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT,v->sampleLocationsPerPixel,v->sampleLocationGridSize.width,v->sampleLocationGridSize.height,v->sampleLocationsCount,PTR(v->pSampleLocations)));}
VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEXT(VkCommandBuffer c,uint32_t factor,uint16_t pattern){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_LINE_STIPPLE_EXT,factor,pattern));}
VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice p,VkSampleCountFlagBits samples,VkMultisamplePropertiesEXT *out){if(!VALID(out,VK_STRUCTURE_TYPE_MULTISAMPLE_PROPERTIES_EXT))checked(VK_ERROR_FEATURE_NOT_PRESENT);checked(CALL(265,KEY(p),samples,PTR(&out->maxSampleLocationGridSize)));}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClipEnableEXT(VkCommandBuffer c,VkBool32 value){checked(CALL(255,DEVKEY(c),KEY(c),VK_DYNAMIC_STATE_DEPTH_CLIP_ENABLE_EXT,value));}
