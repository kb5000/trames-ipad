// SPDX-License-Identifier: GPL-3.0-or-later
// Own each serialized payload until its synchronous host call returns. Batches
// end inside the public Vulkan update; external synchronization stays with the
// caller, and no per-device/global algorithm lock or deferred update is added.
#define DESCRIPTOR_BATCH_CAPABILITY 16384UL
#define DESCRIPTOR_BATCH_COUNT 32
#define DESCRIPTOR_BATCH_WORDS (256*3)
struct descriptor_batch {
    Dispatch *device;
    unsigned count, used;
    uint64_t records[DESCRIPTOR_BATCH_COUNT][9];
    // Bound transport scratch space independently of record count: small
    // updates do not reserve 64 descriptors for every record on guest stacks.
    uint64_t payload[DESCRIPTOR_BATCH_WORDS];
};
static void descriptor_batch_flush(struct descriptor_batch *batch) {
    if(!batch->count)return;
    if(batch->count>1 && (batch->device->capabilities[0]&DESCRIPTOR_BATCH_CAPABILITY))
        checked(CALL(250,batch->device->key,PTR(batch->records),batch->count));
    else for(unsigned i=0;i<batch->count;i++){
        uint64_t args[16]={0};memcpy(args,batch->records[i]+1,8*sizeof(uint64_t));
        checked(call((unsigned)batch->records[i][0],args,__func__));
    }
    batch->count=0;batch->used=0;
}
static void descriptor_batch_write(struct descriptor_batch *batch,const VkWriteDescriptorSet *w,const uint64_t *data) {
    unsigned words=w->descriptorCount*3;
    if(batch->used+words>DESCRIPTOR_BATCH_WORDS)descriptor_batch_flush(batch);
    unsigned slot=batch->count;uint64_t *payload=batch->payload+batch->used;
    memcpy(payload,data,words*sizeof(uint64_t));batch->used+=words;
    uint64_t record[9]={33,batch->device->key,(uint64_t)w->dstSet,w->dstBinding,w->dstArrayElement,w->descriptorCount,w->descriptorType,PTR(payload),0};
    memcpy(batch->records[slot],record,sizeof(record));
    if(++batch->count==DESCRIPTOR_BATCH_COUNT)descriptor_batch_flush(batch);
}
static void descriptor_batch_copy(struct descriptor_batch *batch,const VkCopyDescriptorSet *c) {
    uint64_t record[9]={216,batch->device->key,(uint64_t)c->srcSet,c->srcBinding,c->srcArrayElement,(uint64_t)c->dstSet,c->dstBinding,c->dstArrayElement,c->descriptorCount};
    memcpy(batch->records[batch->count],record,sizeof(record));
    if(++batch->count==DESCRIPTOR_BATCH_COUNT)descriptor_batch_flush(batch);
}
static void descriptor_write(struct descriptor_batch *batch,const VkWriteDescriptorSet *original_write) {
      const VkWriteDescriptorSet *original=original_write;if(!VALID(original,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET) || !original->descriptorCount)checked(VK_ERROR_FEATURE_NOT_PRESENT);
      for(uint32_t offset=0;offset<original->descriptorCount;){
        VkWriteDescriptorSet part=*original;part.descriptorCount=original->descriptorCount-offset;if(part.descriptorCount>64)part.descriptorCount=64;part.dstArrayElement+=offset;
        if(original->pBufferInfo)part.pBufferInfo+=offset;if(original->pImageInfo)part.pImageInfo+=offset;if(original->pTexelBufferView)part.pTexelBufferView+=offset;
        const VkWriteDescriptorSet *w=&part;

        uint64_t buffers[192];
        if(w->descriptorType>=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER && w->descriptorType<=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC){
            if(!w->pBufferInfo)checked(VK_ERROR_FEATURE_NOT_PRESENT);
            for(uint32_t i=0;i<w->descriptorCount;i++){buffers[i*3]=(uint64_t)w->pBufferInfo[i].buffer;buffers[i*3+1]=w->pBufferInfo[i].offset;buffers[i*3+2]=w->pBufferInfo[i].range;}
        }else if(w->descriptorType==VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER || w->descriptorType==VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER){
            if(!w->pTexelBufferView)checked(VK_ERROR_FEATURE_NOT_PRESENT);
            for(uint32_t i=0;i<w->descriptorCount;i++){buffers[i*3]=(uint64_t)w->pTexelBufferView[i];buffers[i*3+1]=0;buffers[i*3+2]=0;}
        }else if(w->descriptorType<=VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || w->descriptorType==VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT){
            if(!w->pImageInfo)checked(VK_ERROR_FEATURE_NOT_PRESENT);
            for(uint32_t i=0;i<w->descriptorCount;i++){buffers[i*3]=w->descriptorType>=VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE?0:(uint64_t)w->pImageInfo[i].sampler;buffers[i*3+1]=w->descriptorType==VK_DESCRIPTOR_TYPE_SAMPLER?0:(uint64_t)w->pImageInfo[i].imageView;buffers[i*3+2]=w->descriptorType==VK_DESCRIPTOR_TYPE_SAMPLER?0:w->pImageInfo[i].imageLayout;}
        }else checked(VK_ERROR_FEATURE_NOT_PRESENT);
        descriptor_batch_write(batch,w,buffers);
        offset+=part.descriptorCount;
      }
}
