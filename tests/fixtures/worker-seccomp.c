// SPDX-License-Identifier: GPL-3.0-or-later
// Source-worker policy publication and steady-entry cost; native Linux oracle.
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#ifdef __linux__
#include <stddef.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#endif
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed=PTHREAD_COND_INITIALIZER;
static unsigned ready,done,started,threads,rounds;
static int mode;
static uint64_t now(void) {
    struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));
    return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;
}
static void *worker(void *unused) {
    (void)unused;
    assert(!pthread_mutex_lock(&lock));ready++;assert(!pthread_cond_broadcast(&changed));
    while(!started)assert(!pthread_cond_wait(&changed,&lock));
    assert(!pthread_mutex_unlock(&lock));
#ifdef __linux__
    if(mode) {
        assert(prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)==1);
        assert(prctl(PR_GET_SECCOMP,0,0,0,0)==2);
    }
#endif
    for(unsigned i=0;i<rounds;i++) {
        long n=syscall(SYS_getppid);
        if(mode==2)assert(n==-1&&errno==42);else assert(n>=0);
    }
    assert(!pthread_mutex_lock(&lock));done++;assert(!pthread_cond_broadcast(&changed));assert(!pthread_mutex_unlock(&lock));
    return NULL;
}
int main(int argc,char **argv) {
    threads=argc>1?(unsigned)strtoul(argv[1],NULL,10):4;
    rounds=argc>2?(unsigned)strtoul(argv[2],NULL,10):5000;
    mode=argc>3?atoi(argv[3]):2;
    assert(threads&&threads<=32&&rounds&&mode>=0&&mode<=2);
    pthread_t ids[32];for(unsigned i=0;i<threads;i++)assert(!pthread_create(&ids[i],NULL,worker,NULL));
    assert(!pthread_mutex_lock(&lock));while(ready!=threads)assert(!pthread_cond_wait(&changed,&lock));
#ifdef __linux__
    if(mode) {
        struct sock_filter code[]={BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(struct seccomp_data,nr)),BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,SYS_getppid,0,1),BPF_STMT(BPF_RET|BPF_K,mode==2?(SECCOMP_RET_ERRNO|42):SECCOMP_RET_ALLOW),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
        struct sock_fprog filter={.len=4,.filter=code};
        assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));
        assert(!syscall(SYS_seccomp,SECCOMP_SET_MODE_FILTER,SECCOMP_FILTER_FLAG_TSYNC,&filter));
    }
#else
    assert(mode==0);
#endif
    uint64_t began=now();started=1;assert(!pthread_cond_broadcast(&changed));
    while(done!=threads)assert(!pthread_cond_wait(&changed,&lock));
    uint64_t elapsed=now()-began;assert(!pthread_mutex_unlock(&lock));
    for(unsigned i=0;i<threads;i++)assert(!pthread_join(ids[i],NULL));
    printf("worker-seccomp: threads=%u rounds=%u mode=%d elapsed_ns=%llu calls=%u: ok\n",threads,rounds,mode,(unsigned long long)elapsed,threads*rounds);
    return 0;
}
