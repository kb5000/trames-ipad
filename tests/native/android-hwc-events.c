// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../accelerator/android/hwc-events.h"
#include "../../accelerator/android/hwc-crop.h"
#include <assert.h>
#include <stdio.h>
struct fixture {struct trames_hwc_events events;bool enabled,reenter;unsigned reconnects,vsyncs;pthread_t callback_thread;};
static void enabled(void *p,bool on){((struct fixture *)p)->enabled=on;}
static void reconnect(void *p){struct fixture *f=p;f->reconnects++;f->callback_thread=pthread_self();if(f->reenter){f->reenter=false;trames_hwc_enable(&f->events,false);trames_hwc_request(&f->events);}}
static void vsync(void *p,int64_t time){assert(time>0);((struct fixture *)p)->vsyncs++;}
static void *callback(void *p){struct fixture *f=p;trames_hwc_vsync(&f->events,1);return NULL;}
int main(void){
    float crop[4]={0,0,2176,1560};assert(trames_hwc_crop(crop,2092,1544));assert(crop[2]==2092 && crop[3]==1544);
    float old[4]={0,0,3824,1852};assert(trames_hwc_crop(old,1280,960));assert(old[2]==1280 && old[3]==960);
    float valid[4]={5,7,100,90};assert(!trames_hwc_crop(valid,1280,960));assert(valid[0]==5 && valid[3]==90);
    struct fixture f={.events={.lock=PTHREAD_MUTEX_INITIALIZER,.enable=enabled,.reconnect=reconnect,.vsync=vsync}};f.events.owner=&f;
    trames_hwc_enable(&f.events,false);assert(!f.enabled);
    for(int i=0;i<20;i++)trames_hwc_request(&f.events);
    assert(f.enabled && !f.reconnects); // No callback on the requesting thread.
    pthread_t thread;assert(!pthread_create(&thread,NULL,callback,&f));assert(!pthread_join(thread,NULL));
    assert(f.reconnects==1 && !f.enabled && !f.vsyncs && !pthread_equal(f.callback_thread,pthread_self()));
    trames_hwc_enable(&f.events,true);trames_hwc_vsync(&f.events,2);assert(f.vsyncs==1);
    f.reenter=true;trames_hwc_request(&f.events);trames_hwc_vsync(&f.events,3);
    assert(f.enabled && f.reconnects==2 && f.vsyncs==1); // Reentrant request survives.
    trames_hwc_vsync(&f.events,4);assert(!f.enabled && f.reconnects==3 && f.vsyncs==1);
    trames_hwc_vsync(&f.events,5);assert(f.reconnects==3);
    pthread_mutex_destroy(&f.events.lock);puts("android-hwc-events-ok");return 0;
}
