// SPDX-License-Identifier: GPL-3.0-or-later
// Application allocators execute only as ordinary guest C calls on the caller's
// thread. Host-side GPU/transport allocations never receive guest callbacks.
struct object_allocation {
    ObjectAllocation *next;
    uint64_t key;
    VkAllocationCallbacks callbacks;
};
static void allocation_free(ObjectAllocation *record) {
    VkAllocationCallbacks callbacks=record->callbacks;
    callbacks.pfnFree(callbacks.pUserData,record);
}
static void allocations_clear(Dispatch *owner) {
    ObjectAllocation *p=owner->allocations;owner->allocations=NULL;
    while(p){ObjectAllocation *next=p->next;allocation_free(p);p=next;}
}
static void allocation_release(Dispatch *owner,uint64_t key) {
    ObjectAllocation *found=NULL;
    pthread_mutex_lock(&owner->lock);
    for(ObjectAllocation **p=&owner->allocations;*p;p=&(*p)->next)if((*p)->key==key){found=*p;*p=found->next;break;}
    pthread_mutex_unlock(&owner->lock);
    if(found)allocation_free(found);
}
static Dispatch *dispatch_with_allocator(uint64_t key,Dispatch *device,const VkAllocationCallbacks *callbacks,VkSystemAllocationScope scope) {
    Dispatch *d=callbacks?callbacks->pfnAllocation(callbacks->pUserData,sizeof(*d),_Alignof(Dispatch),scope):malloc(sizeof(*d));
    if(!d)return NULL;
    memset(d,0,sizeof(*d));set_loader_magic_value(d);d->key=key;d->device=device;
    d->allocation_scope=scope;if(callbacks){d->callbacks=*callbacks;d->custom_allocator=1;}
    pthread_mutex_init(&d->lock,NULL);return d;
}
static const VkAllocationCallbacks *dispatch_allocator(Dispatch *d) {return d->custom_allocator?&d->callbacks:NULL;}
struct allocation_guard {Dispatch *owner;void *output;ObjectAllocation *record;VkSystemAllocationScope scope;};
static struct allocation_guard allocation_begin(Dispatch *owner,const VkAllocationCallbacks *callbacks,void *output,VkSystemAllocationScope scope) {
    struct allocation_guard guard={owner,output,NULL,scope};
    if(!callbacks && owner->custom_allocator){callbacks=&owner->callbacks;scope=owner->allocation_scope;}
    guard.scope=scope;
    if(callbacks){
        guard.record=callbacks->pfnAllocation(callbacks->pUserData,sizeof(ObjectAllocation),_Alignof(ObjectAllocation),scope);
        if(guard.record){memset(guard.record,0,sizeof(*guard.record));guard.record->callbacks=*callbacks;}
    }
    return guard;
}
static void allocation_finish(struct allocation_guard *guard) {
    if(!guard->record)return;
    uint64_t key;memcpy(&key,guard->output,sizeof(key));
    if(!key){allocation_free(guard->record);return;}
    guard->record->key=key;
    pthread_mutex_lock(&guard->owner->lock);
    guard->record->next=guard->owner->allocations;guard->owner->allocations=guard->record;
    pthread_mutex_unlock(&guard->owner->lock);
}
// The output is published only by a successful native creation. The cleanup
// owns either the original allocation or its linked object record on all exits.
#define OBJECT_ALLOCATOR(owner,p,scope) \
    memset(out,0,sizeof(*out)); \
    struct allocation_guard allocation __attribute__((cleanup(allocation_finish)))=allocation_begin(D(owner),p,out,scope); \
    if(((p) || D(owner)->custom_allocator) && !allocation.record)return VK_ERROR_OUT_OF_HOST_MEMORY
static int pool_allocator(Dispatch *device,uint64_t pool,VkAllocationCallbacks *out) {
    int found=0;pthread_mutex_lock(&device->lock);
    for(ObjectAllocation *p=device->allocations;p;p=p->next)if(p->key==pool){*out=p->callbacks;found=1;break;}
    pthread_mutex_unlock(&device->lock);
    if(!found && device->custom_allocator){*out=device->callbacks;found=1;}
    return found;
}
