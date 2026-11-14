// SPDX-License-Identifier: GPL-3.0-or-later
// Waydroid primary-display reconnects belong on its existing vsync callback
// thread, never inline in SurfaceFlinger's unlocked composition call.
#ifndef TRAMES_ANDROID_HWC_EVENTS_H
#define TRAMES_ANDROID_HWC_EVENTS_H
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
struct trames_hwc_events {
    pthread_mutex_t lock;
    atomic_bool pending, wanted;
    bool delivering;
    void *owner;
    void (*enable)(void *, bool);
    void (*reconnect)(void *);
    void (*vsync)(void *, int64_t);
};
static void trames_hwc_request(struct trames_hwc_events *e) {
    pthread_mutex_lock(&e->lock);
    atomic_store_explicit(&e->pending,true,memory_order_relaxed);
    // Wake the original HWC's callback source even if SF disabled idle vsync.
    e->enable(e->owner,true);
    pthread_mutex_unlock(&e->lock);
}
static void trames_hwc_enable(struct trames_hwc_events *e,bool wanted) {
    pthread_mutex_lock(&e->lock);
    atomic_store_explicit(&e->wanted,wanted,memory_order_relaxed);
    e->enable(e->owner,wanted || e->delivering || atomic_load_explicit(&e->pending,memory_order_relaxed));
    pthread_mutex_unlock(&e->lock);
}
static void trames_hwc_vsync(struct trames_hwc_events *e,int64_t time) {
    if(atomic_load_explicit(&e->pending,memory_order_relaxed)) {
        pthread_mutex_lock(&e->lock);
        bool deliver=atomic_exchange_explicit(&e->pending,false,memory_order_relaxed);
        if(deliver)e->delivering=true;
        pthread_mutex_unlock(&e->lock);
        if(deliver) {
            // SF takes its own state lock here. Never hold our event lock
            // across that callback, which can itself change vsync policy.
            e->reconnect(e->owner);
            pthread_mutex_lock(&e->lock);
            e->delivering=false;
            e->enable(e->owner,atomic_load_explicit(&e->wanted,memory_order_relaxed) || atomic_load_explicit(&e->pending,memory_order_relaxed));
            pthread_mutex_unlock(&e->lock);
        }
    }
    if(atomic_load_explicit(&e->wanted,memory_order_relaxed))e->vsync(e->owner,time);
}
#endif
