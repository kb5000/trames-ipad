// SPDX-License-Identifier: GPL-3.0-or-later
// Recording is externally synchronized by Vulkan's command-buffer/pool owner.
// Keep the pending packet on that object, never TLS or a device-wide queue.
#define COMMAND_BATCH_CAPABILITY (1UL<<16)
#define COMMAND_BATCH_OPERATION 276
#define COMMAND_BATCH_COUNT 64
#define COMMAND_BATCH_BYTES 16384
struct command_batch {
    unsigned count, used;
    uint64_t records[COMMAND_BATCH_COUNT][17];
    uint64_t payload[COMMAND_BATCH_BYTES/8];
};
static void command_batch_free(Dispatch *c) {
    if(!c->batch)return;
    if(c->custom_allocator)c->callbacks.pfnFree(c->callbacks.pUserData,c->batch);
    else free(c->batch);
    c->batch=NULL;
}
static VkResult command_batch_flush(Dispatch *c) {
    CommandBatch *b=c->batch;
    if(!b || !b->count)return VK_SUCCESS;
    unsigned count=b->count,used=b->used;
    // A failed publication is never replayed, including a partially accepted
    // invalid application command sequence. The original checks remain host-owned.
    b->count=0;b->used=8;
    return CALL(COMMAND_BATCH_OPERATION,DEVKEY(c),c->key,PTR(b->records),count,PTR(b->payload),used);
}
// Payload references are byte offsets in one owned packet. Offset zero is NULL.
// No application pointer, stack scratch or serializer arena survives the call.
static int command_span(CommandBatch *b,uint64_t *pointer,uint64_t count,size_t width) {
    if(!count){*pointer=0;return 1;}
    if(!*pointer || !width || count>COMMAND_BATCH_BYTES/width)return 0;
    size_t bytes=(size_t)count*width,at=(b->used+7u)&~7u;
    if(bytes>COMMAND_BATCH_BYTES-at)return 0;
    memcpy((unsigned char *)b->payload+at,(const void *)(uintptr_t)*pointer,bytes);
    *pointer=at;b->used=(unsigned)(at+bytes);return 1;
}
static uint64_t *command_header(CommandBatch *b,uint64_t *pointer,size_t words) {
    if(!command_span(b,pointer,words,8))return NULL;
    return (void *)((unsigned char *)b->payload+*pointer);
}
static int command_dependency(CommandBatch *b,uint64_t h[7]) {
    return command_span(b,h+2,h[1],4*8) && command_span(b,h+4,h[3],9*8) && command_span(b,h+6,h[5],14*8);
}
static int command_copy(CommandBatch *b,unsigned op,uint64_t a[16]) {
    switch(op){
    case 41:case 43:case 46:case 72:case 73:case 76:case 96:case 97:
    case 124:case 140:case 142:case 143:case 144:case 145:case 149:
    case 203:case 204:case 205:case 210:case 211:case 218:
    case 225:case 226:case 235:case 236:case 254:case 268:case 270:return 1;
    case 42:return command_span(b,a+4,a[5],8) && command_span(b,a+7,a[8],4);
    case 44:return command_span(b,a+6,a[7],(size_t)a[8]*8) && command_span(b,a+10,a[11],7*8);
    case 45:return command_span(b,a+4,a[5],3*8);
    case 47:return command_span(b,a+5,a[4],1);
    case 71:case 123:return command_span(b,a+8,a[9],16) && command_span(b,a+10,a[11],8);
    case 74:case 94:return command_span(b,a+5,a[6],13*8);
    case 75:return command_span(b,a+2,1,sizeof(VkViewport));
    case 95:return command_span(b,a+4,a[3],8) && command_span(b,a+5,a[3],8);
    case 114:return command_span(b,a+4,1,16) && command_span(b,a+5,a[6],5*8);
    case 132:return command_span(b,a+3,a[4],4);
    case 134:case 215:return command_span(b,a+6,a[7],17*8);
    case 135:return command_span(b,a+6,a[7],20*8);
    case 136:return command_span(b,a+4,1,sizeof(VkClearDepthStencilValue)) && command_span(b,a+5,a[6],5*8);
    case 146:return command_span(b,a+4,a[3],8) && command_span(b,a+5,a[3],8) && (!a[6] || command_span(b,a+6,a[3],8));
    case 147:case 148:return (!a[4] || command_span(b,a+4,a[3],8)) && (!a[5] || command_span(b,a+5,a[3],8));
    case 151:return command_span(b,a+2,a[3],6*8) && command_span(b,a+4,a[5],6*8);
    case 213:return command_span(b,a+5,a[4],1);
    case 219:return command_span(b,a+2,a[3],8);
    case 227:{
        uint64_t *h=command_header(b,a+2,10);
        return h && command_span(b,h+1,h[0],8) && command_span(b,h+5,h[4],2*8) && command_span(b,h+7,h[6],7*8) && command_span(b,h+9,h[8],10*8);
    }
    case 253:{
        uint64_t *h=command_header(b,a+2,11);
        return h && command_span(b,h+8,h[7],9*8) && (!h[9] || command_span(b,h+9,1,9*8)) && (!h[10] || command_span(b,h+10,1,9*8)) && command_span(b,a+4,a[5],4*8);
    }
    case 255:
        switch((VkDynamicState)a[2]){
        case VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT:return command_span(b,a+4,a[3],sizeof(VkViewport));
        case VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT:return command_span(b,a+4,a[3],sizeof(VkRect2D));
        case VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE:return command_span(b,a+5,a[4],8) && command_span(b,a+6,a[4],8) && (!a[7] || command_span(b,a+7,a[4],8)) && (!a[8] || command_span(b,a+8,a[4],8));
        case VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT:return command_span(b,a+7,a[6],sizeof(VkSampleLocationEXT));
        default:return 1;
        }
    case 266:case 267:{uint64_t *h=command_header(b,a+(op==266?2:3),7);return h && command_dependency(b,h);}
    case 269:{
        uint64_t *h=command_header(b,a+3,(size_t)a[4]*7);
        if(a[4] && !h)return 0;
        for(uint64_t i=0;i<a[4];i++)if(!command_dependency(b,h+i*7))return 0;
        return command_span(b,a+2,a[4],8);
    }
    default:return 0;
    }
}
static VkResult command_call(Dispatch *c,unsigned op,uint64_t a[16],const char *name) {
    if(!(c->device->capabilities[0]&COMMAND_BATCH_CAPABILITY))return call(op,a,name);
    // All supported recorders use COMMAND; begin/end/reset reach this same
    // entry and flush before invoking their original immediate operation.
    if(!((op>=41 && op<=47) || (op>=71 && op<=76) || (op>=94 && op<=97) || op==114 || op==123 || op==124 || op==132 || (op>=134 && op<=136) || op==140 || (op>=142 && op<=149) || op==151 || (op>=203 && op<=205) || op==210 || op==211 || op==213 || op==215 || op==218 || op==219 || op==225 || op==226 || op==227 || op==235 || op==236 || (op>=253 && op<=255) || (op>=266 && op<=270))){
        VkResult r=command_batch_flush(c);return r==VK_SUCCESS?call(op,a,name):r;
    }
    if(!c->batch){
        c->batch=c->custom_allocator?c->callbacks.pfnAllocation(c->callbacks.pUserData,sizeof(*c->batch),_Alignof(CommandBatch),VK_SYSTEM_ALLOCATION_SCOPE_COMMAND):malloc(sizeof(*c->batch));
        if(!c->batch)return call(op,a,name);
        c->batch->count=0;c->batch->used=8;c->batch->payload[0]=0;
    }
    CommandBatch *b=c->batch;
    for(unsigned attempt=0;attempt<2;attempt++){
        uint64_t owned[16];memcpy(owned,a,sizeof(owned));unsigned before=b->used;
        if(command_copy(b,op,owned)){
            uint64_t *row=b->records[b->count++];row[0]=op;memcpy(row+1,owned,sizeof(owned));
            return b->count==COMMAND_BATCH_COUNT?command_batch_flush(c):VK_SUCCESS;
        }
        b->used=before;
        VkResult r=command_batch_flush(c);if(r!=VK_SUCCESS)return r;
    }
    // Oversized payloads keep the original immediate path and limits. Previous
    // records are already published, so no copy failure can reorder commands.
    return call(op,a,name);
}
static VkResult command_pool_flush(Dispatch *d,uint64_t pool) {
    size_t done=0;
    for(;;){
        Dispatch *part[64];unsigned n=0;size_t seen=0;
        // The pool is externally synchronized. Other pools may change the
        // device list; retain only this pool's stable pointers under its
        // existing list lock, then release it before every host invocation.
        pthread_mutex_lock(&d->lock);
        for(Dispatch *c=d->commands;c && n<64;c=c->next)
            if(c->parent==pool && seen++>=done)part[n++]=c;
        pthread_mutex_unlock(&d->lock);
        if(!n)return VK_SUCCESS;
        for(unsigned i=0;i<n;i++){VkResult r=command_batch_flush(part[i]);if(r!=VK_SUCCESS)return r;}
        done+=n;
    }
}
#define COMMAND(op,c,...) command_call(D(c),op,(uint64_t[16]){DEVKEY(c),KEY(c),##__VA_ARGS__},__func__)
