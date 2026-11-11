// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the real ICD serializers with a synchronous recording transport.
#define _GNU_SOURCE
#include <assert.h>
#ifdef __APPLE__
#define MFD_CLOEXEC 1
static int memfd_create(const char *name,unsigned flags){(void)name;(void)flags;assert(!"unused Wayland path");return -1;}
#endif
#include "../../accelerator/vulkan/icd.c"
static uint64_t saved[512][3];
static unsigned native_calls;
unsigned long getauxval(unsigned long key){return key==0x54520008UL?0x10006UL:0;}
static void apply_record(unsigned op,const uint64_t *a){
    assert(a[0]==7);
    if(op==33){
        const uint64_t *data=(const void *)(uintptr_t)a[6];
        for(unsigned i=0;i<a[4];i++)memcpy(saved[a[2]+a[3]+i],data+i*3,24);
    }else{assert(op==216);for(unsigned i=0;i<a[7];i++)memcpy(saved[a[5]+a[6]+i],saved[a[2]+a[3]+i],24);}
}
struct result trames_vulkan_call(uint64_t op,const uint64_t *a,size_t size){
    assert(size==128);native_calls++;
    if(op==250){
        assert(a[0]==7 && a[2]>1 && a[2]<=32);const uint64_t *rows=(const void *)(uintptr_t)a[1];
        for(unsigned i=0;i<a[2];i++)apply_record((unsigned)rows[i*9],rows+i*9+1);
    }else apply_record((unsigned)op,a);
    return (struct result){0,0};
}
static void run(int fast){
    Dispatch d={.key=7,.capabilities={fast?DESCRIPTOR_BATCH_CAPABILITY:0},.lock=PTHREAD_MUTEX_INITIALIZER};d.device=&d;
    VkDevice device=(void *)&d;VkWriteDescriptorSet writes[72];VkDescriptorBufferInfo data[300];
    for(unsigned i=0;i<300;i++)data[i]=(VkDescriptorBufferInfo){.buffer=(VkBuffer)(uintptr_t)(1000+i),.offset=i*16,.range=32};
    for(unsigned i=0;i<72;i++)writes[i]=(VkWriteDescriptorSet){.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,.dstSet=(VkDescriptorSet)1,.dstBinding=i,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.pBufferInfo=data+i};
    VkCopyDescriptorSet copies[2]={{.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,.srcSet=(VkDescriptorSet)1,.dstSet=(VkDescriptorSet)1,.srcBinding=0,.dstBinding=100,.descriptorCount=1},{.sType=VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,.srcSet=(VkDescriptorSet)1,.dstSet=(VkDescriptorSet)1,.srcBinding=100,.dstBinding=101,.descriptorCount=1}};
    native_calls=0;memset(saved,0,sizeof(saved));vkUpdateDescriptorSets(device,72,writes,2,copies);
    assert(native_calls==(fast?3:74));for(unsigned i=0;i<72;i++)assert(saved[i][0]==1000+i && saved[i][1]==i*16);assert(saved[100][0]==1000 && saved[101][0]==1000);
    native_calls=0;writes[0].descriptorCount=130;vkUpdateDescriptorSets(device,1,writes,0,NULL);
    assert(native_calls==(fast?1:3));for(unsigned i=0;i<130;i++)assert(saved[i][0]==1000+i && saved[i][1]==i*16);
    native_calls=0;writes[0].descriptorCount=300;vkUpdateDescriptorSets(device,1,writes,0,NULL);
    assert(native_calls==(fast?2:5));for(unsigned i=0;i<300;i++)assert(saved[i][0]==1000+i && saved[i][1]==i*16);writes[0].descriptorCount=1;
    VkDescriptorImageInfo images[3]={{.sampler=(VkSampler)11},{.imageView=(VkImageView)22,.imageLayout=VK_IMAGE_LAYOUT_GENERAL},{.sampler=(VkSampler)33,.imageView=(VkImageView)44,.imageLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkBufferView view=(VkBufferView)55;VkWriteDescriptorSet mixed[4];
    for(unsigned i=0;i<4;i++){mixed[i]=writes[0];mixed[i].dstBinding=i;mixed[i].pBufferInfo=NULL;mixed[i].descriptorType=i<3?(VkDescriptorType[]){VK_DESCRIPTOR_TYPE_SAMPLER,VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}[i]:VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;mixed[i].pImageInfo=i<3?images+i:NULL;mixed[i].pTexelBufferView=i==3?&view:NULL;}
    native_calls=0;vkUpdateDescriptorSets(device,4,mixed,0,NULL);assert(native_calls==(fast?1:4));
    assert(saved[0][0]==11 && saved[0][1]==0 && saved[0][2]==0);assert(saved[1][0]==0 && saved[1][1]==22 && saved[1][2]==VK_IMAGE_LAYOUT_GENERAL);assert(saved[2][0]==33 && saved[2][1]==44);assert(saved[3][0]==55 && saved[3][1]==0 && saved[3][2]==0);
    VkDescriptorUpdateTemplateEntry entries[72];
    struct padded{uint64_t pad[3];VkDescriptorBufferInfo value;} padded[72];
    for(unsigned i=0;i<72;i++){padded[i].value=data[i];entries[i]=(VkDescriptorUpdateTemplateEntry){.dstBinding=i,.descriptorCount=1,.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,.offset=i*sizeof(*padded)+offsetof(struct padded,value),.stride=sizeof(*padded)};}
    VkDescriptorUpdateTemplateCreateInfo info={.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,.descriptorUpdateEntryCount=72,.pDescriptorUpdateEntries=entries,.templateType=VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET,.descriptorSetLayout=(VkDescriptorSetLayout)1};
    VkDescriptorUpdateTemplate t;assert(vkCreateDescriptorUpdateTemplate(device,&info,NULL,&t)==VK_SUCCESS);native_calls=0;memset(saved,0,sizeof(saved));vkUpdateDescriptorSetWithTemplate(device,(VkDescriptorSet)1,t,padded);
    assert(native_calls==(fast?3:72));for(unsigned i=0;i<72;i++)assert(saved[i][0]==1000+i && saved[i][1]==i*16);vkDestroyDescriptorUpdateTemplate(device,t,NULL);
    native_calls=0;vkUpdateDescriptorSets(device,0,NULL,0,NULL);assert(native_calls==0);vkUpdateDescriptorSets(device,1,writes,0,NULL);assert(native_calls==1);
    pthread_mutex_destroy(&d.lock);
}
int main(void){run(0);run(1);puts("descriptor batch: payload lifetime, >32 records, >64 elements, ordered copies, padded templates and legacy fallback passed (72 writes: 72 -> 3 native calls)");}
