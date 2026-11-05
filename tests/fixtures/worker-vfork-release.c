// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line=%d %s errno=%d\n",__LINE__,#x,errno); _exit(1); } } while (0)
enum { STACK = 256 * 1024 };
struct context { _Atomic int go, expired; int owner; void *peer_stack; };
static uint64_t now(void) {
    struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);
    return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;
}
static int peer(void *opaque) {
    struct context *ctx=opaque;
    uint64_t end=now()+2000000000ull;
    unsigned spins=0;
    while (!atomic_load(&ctx->go)) {
        if (++spins==1024) {
            spins=0;
            if(now()>=end) {atomic_store(&ctx->expired,1);break;}
        }
    }
    syscall(SYS_exit,atomic_load(&ctx->expired)?82:0);
    __builtin_unreachable();
}
static int child(void *opaque) {
    struct context *ctx=opaque;
    // Keep another CPU executing in this group while the actual vfork child
    // exits. The parent must resume before that peer can see go=1.
    CHECK(clone(peer,(char*)ctx->peer_stack+STACK,CLONE_VM|CLONE_SIGHAND|CLONE_THREAD,ctx)>0);
    // Only this task owns the old control's projection: the peer copied Files
    // before it was created. Its destructor must run on the original owner.
    if(ctx->owner) CHECK(epoll_create1(EPOLL_CLOEXEC)>=0);
    syscall(SYS_exit,0);
    __builtin_unreachable();
}
int main(int argc,char **argv) {
    int rounds=argc>1?atoi(argv[1]):8; CHECK(rounds>0 && rounds<=10000);
    uint64_t total[2]={0,0}, maximum[2]={0,0};
    for(int i=0;i<rounds;i++) for(int owner=0;owner<2;owner++) {
        void *stack=mmap(0,STACK,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        void *other=mmap(0,STACK,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        CHECK(stack!=MAP_FAILED && other!=MAP_FAILED);
        struct context ctx={.owner=owner,.peer_stack=other};
        uint64_t begin=now();
        int pid=clone(child,(char*)stack+STACK,CLONE_VM|CLONE_VFORK|SIGCHLD,&ctx);
        uint64_t elapsed=now()-begin;
        atomic_store(&ctx.go,1);
        CHECK(pid>0);
        int status=0; CHECK(waitpid(pid,&status,0)==pid);
        CHECK(!atomic_load(&ctx.expired)); CHECK(status==0);
        total[owner]+=elapsed; if(elapsed>maximum[owner])maximum[owner]=elapsed;
        CHECK(munmap(stack,STACK)==0 && munmap(other,STACK)==0);
    }
    printf("{\"case\":\"worker-vfork-release\",\"rounds\":%d,\"plain_mean_ns\":%llu,\"owner_mean_ns\":%llu,\"plain_max_ns\":%llu,\"owner_max_ns\":%llu,\"passed\":true}\n",
        rounds,(unsigned long long)(total[0]/rounds),(unsigned long long)(total[1]/rounds),
        (unsigned long long)maximum[0],(unsigned long long)maximum[1]);
    return 0;
}
