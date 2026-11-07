// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/epoll.h>
#include <sys/syscall.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#endif
static unsigned threads,rounds,mode,policy;
static pthread_barrier_t barrier;
static int pipefd[2];
static uint64_t now(void) {struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static void once(void) {
    struct stat info;
#ifdef __linux__
    int ep=(int)syscall(SYS_epoll_create1,EPOLL_CLOEXEC);assert(ep>=0);
    assert(!syscall(SYS_fstat,ep,&info));
    assert(!syscall(SYS_fstat,1,&info));
    assert(syscall(SYS_lseek,pipefd[0],0,SEEK_SET)==-1 && errno==ESPIPE);
    assert(!close(ep));
#else
    assert(!fstat(1,&info));assert(lseek(pipefd[0],0,SEEK_SET)==-1 && errno==ESPIPE);
#endif
    if (mode==1) {
        assert(write(1,".",1)==1);
        struct iovec v[2]={{".",1},{".",1}};assert(writev(1,v,2)==2);
    } else if (mode==2) {
        int s=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);assert(s>=0);
        int yes=1;assert(!setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes));
        socklen_t n=sizeof yes;assert(!getsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,&n) && yes);
        assert(!shutdown(s,SHUT_RDWR) || errno==ENOTCONN);assert(!close(s));
    }
}
static void *worker(void *arg) {
    (void)arg;
    pthread_barrier_wait(&barrier);
    for(unsigned i=0;i<rounds;i++)once();
    return NULL;
}
int main(int argc,char **argv) {
    threads=argc>1?(unsigned)atoi(argv[1]):4;rounds=argc>2?(unsigned)atoi(argv[2]):200;
    mode=argc>3?(unsigned)atoi(argv[3]):0;policy=argc>4?(unsigned)atoi(argv[4]):0;
    assert(threads>0 && threads<=32 && rounds>0);assert(!pipe(pipefd));
#ifdef __linux__
    if(policy) {
        struct sock_filter filter[]={BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
        struct sock_fprog program={1,filter};assert(!prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0));assert(!prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program));
    }
#endif
    assert(!pthread_barrier_init(&barrier,NULL,threads+1));
    pthread_t tasks[32];for(unsigned i=0;i<threads;i++)assert(!pthread_create(&tasks[i],NULL,worker,NULL));
    uint64_t began=now();pthread_barrier_wait(&barrier);
    for(unsigned i=0;i<threads;i++)assert(!pthread_join(tasks[i],NULL));
    uint64_t elapsed=now()-began;
    assert(!close(pipefd[0]));assert(!close(pipefd[1]));
    printf("\nfd-compat threads=%u rounds=%u mode=%u policy=%u elapsed_ns=%llu: ok\n",threads,rounds,mode,policy,(unsigned long long)elapsed);
    return 0;
}
