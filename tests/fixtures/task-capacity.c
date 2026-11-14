// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <linux/futex.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

static _Atomic int released;
static _Atomic int arrived;
static _Atomic int finished;
static void *waiter(void *unused) {
    (void)unused;
    atomic_fetch_add_explicit(&arrived,1,memory_order_release);
    syscall(SYS_futex,&arrived,FUTEX_WAKE_PRIVATE,1,NULL,NULL,0);
    while(!atomic_load_explicit(&released,memory_order_acquire)) {
        long r=syscall(SYS_futex,&released,FUTEX_WAIT_PRIVATE,0,NULL,NULL,0);
        assert(r==0 || errno==EAGAIN || errno==EINTR);
    }
    atomic_fetch_add_explicit(&finished,1,memory_order_release);
    return NULL;
}
int main(int argc,char **argv) {
    unsigned count=argc>1?(unsigned)strtoul(argv[1],NULL,10):512;
    assert(count>256 && count<=4096);
    pthread_t *threads=calloc(count,sizeof(*threads));assert(threads);
    pthread_attr_t attr;assert(!pthread_attr_init(&attr));assert(!pthread_attr_setstacksize(&attr,128*1024));
    for(unsigned round=0;round<4;round++) {
        atomic_store_explicit(&released,0,memory_order_relaxed);
        atomic_store_explicit(&arrived,0,memory_order_relaxed);
        atomic_store_explicit(&finished,0,memory_order_relaxed);
        unsigned created=0;
        for(;created<count;created++) {
            int r=pthread_create(threads+created,&attr,waiter,NULL);
            if(r) {
                fprintf(stderr,"task-capacity-failed: round=%u created=%u errno=%d entered=%d\n",round,created,r,atomic_load_explicit(&arrived,memory_order_acquire));
                atomic_store_explicit(&released,1,memory_order_release);syscall(SYS_futex,&released,FUTEX_WAKE_PRIVATE,INT_MAX,NULL,NULL,0);
                for(unsigned i=0;i<created;i++)assert(!pthread_join(threads[i],NULL));
                free(threads);pthread_attr_destroy(&attr);return 1;
            }
        }
        for(;;) {
            int value=atomic_load_explicit(&arrived,memory_order_acquire);
            if((unsigned)value==count)break;
            long r=syscall(SYS_futex,&arrived,FUTEX_WAIT_PRIVATE,value,NULL,NULL,0);
            assert(r==0 || errno==EAGAIN || errno==EINTR);
        }
        atomic_store_explicit(&released,1,memory_order_release);syscall(SYS_futex,&released,FUTEX_WAKE_PRIVATE,INT_MAX,NULL,NULL,0);
        for(unsigned i=0;i<count;i++)assert(!pthread_join(threads[i],NULL));
        assert((unsigned)atomic_load_explicit(&finished,memory_order_acquire)==count);
        printf("task-capacity-round:%u live=%u reaped=%u\n",round,count+1,count);
    }
    pthread_attr_destroy(&attr);free(threads);puts("task-capacity-ok: concurrent creation and repeated reuse above 256");return 0;
}
