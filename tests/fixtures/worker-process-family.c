// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum {FUTEX_WAIT=0,FUTEX_WAKE=1,SECCOMP_SET_MODE_FILTER=1};
struct sock_filter {uint16_t code;uint8_t jt,jf;uint32_t k;};
struct sock_fprog {unsigned short len;struct sock_filter *filter;};

#define CHECK(x) do {if (!(x)) {fprintf(stderr,"FAIL line=%d %s errno=%d\n",__LINE__,#x,errno);exit(1);}} while(0)
static long call(long nr,long a0,long a1,long a2,long a3,long a4,long a5) {
    register long x0 __asm__("x0")=a0,x1 __asm__("x1")=a1,x2 __asm__("x2")=a2;
    register long x3 __asm__("x3")=a3,x4 __asm__("x4")=a4,x5 __asm__("x5")=a5,x8 __asm__("x8")=nr;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8) : "memory","cc");
    return x0;
}
static long control(long op,long value) {return call(SYS_prctl,op,value,0,0,0,0);}
static long expected_parent;
static uint64_t now(void) {struct timespec ts;CHECK(clock_gettime(CLOCK_MONOTONIC,&ts)==0);return ts.tv_sec*1000000000ull+ts.tv_nsec;}
static void wake(_Atomic int *p) {call(SYS_futex,(long)p,FUTEX_WAKE,INT_MAX,0,0,0);}
static void wait_value(_Atomic int *p,int expected) {
    // A timeout prevents a failed clear_tid from hanging native acceptance.
    struct timespec timeout={1,0};
    long result=call(SYS_futex,(long)p,FUTEX_WAIT,expected,(long)&timeout,0,0);
    CHECK(result==0 || result==-EAGAIN || result==-EINTR || result==-ETIMEDOUT);
}
static void join(_Atomic int *p) {
    uint64_t end=now()+5000000000ull;
    for (;;) {int tid=atomic_load(p);if(!tid)return;CHECK(now()<end);wait_value(p,tid);}
}
struct task {
    _Atomic int tid,start,go,error;
    int rounds,mode;
    void *stack;
};
static int child(void *opaque) {
    struct task *p=opaque;
    if(p->mode) {
        struct sock_filter insns[]={{0x20,0,0,0},{0x15,0,1,p->mode==1?SYS_prctl:SYS_getpgid},{6,0,0,0x50000|EPERM},{6,0,0,0x7fff0000}};
        struct sock_fprog program={.len=4,.filter=insns};
        if(control(PR_SET_NO_NEW_PRIVS,1)!=0 || call(SYS_seccomp,SECCOMP_SET_MODE_FILTER,0,(long)&program,0,0,0)!=0)atomic_store(&p->error,1);
        int value=0x55;
        if(p->mode==1) {
            if(control(PR_SET_CHILD_SUBREAPER,0)!=-EPERM || control(PR_GET_CHILD_SUBREAPER,(long)&value)!=-EPERM || value!=0x55)atomic_store(&p->error,2);
        } else if(control(PR_SET_CHILD_SUBREAPER,1)!=0 || control(PR_GET_CHILD_SUBREAPER,(long)&value)!=0 || value!=1)atomic_store(&p->error,3);
        if(call(SYS_getppid,0,0,0,0,0,0)!=expected_parent)atomic_store(&p->error,4);
        return 0;
    }
    atomic_store(&p->start,1);wake(&p->start);
    while(!atomic_load(&p->go))wait_value(&p->go,0);
    for(int i=0;i<p->rounds;i++) {
        int value=0;
        if(control(PR_SET_CHILD_SUBREAPER,1)!=0 || control(PR_GET_CHILD_SUBREAPER,(long)&value)!=0 || value!=1 || call(SYS_getppid,0,0,0,0,0,0)!=expected_parent)atomic_store(&p->error,5);
    }
    if(control(PR_GET_CHILD_SUBREAPER,1)!=-EFAULT)atomic_store(&p->error,6);
    return 0;
}
static void start(struct task *p) {
    p->stack=mmap(NULL,1<<20,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);CHECK(p->stack!=MAP_FAILED);
    int flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID;
    CHECK(clone(child,(char*)p->stack+(1<<20),flags,p,&p->tid,NULL,&p->tid)>0);
}
static void done(struct task *p) {join(&p->tid);CHECK(!atomic_load(&p->error));CHECK(munmap(p->stack,1<<20)==0);}
int main(int argc,char **argv) {
    int bench=argc>1 && argv[1][0]=='b';int rounds=argc>2?atoi(argv[2]):64;CHECK(rounds>0 && rounds<=1000000);
    expected_parent=getppid();
    CHECK(prctl(PR_SET_CHILD_SUBREAPER,1)==0);
    if(!bench) {
        struct task denied={.mode=1},allowed={.mode=2};
        start(&denied);done(&denied);start(&allowed);done(&allowed);
        int value=0;CHECK(prctl(PR_GET_CHILD_SUBREAPER,&value)==0 && value==1);
        pid_t pid=fork();CHECK(pid>=0);
        if(!pid) {value=7;CHECK(prctl(PR_GET_CHILD_SUBREAPER,&value)==0 && value==0);_exit(0);}
        int status;CHECK(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0);
    }
    struct task tasks[4]={0};
    for(int i=0;i<4;i++){tasks[i].rounds=rounds;start(&tasks[i]);}
    for(int i=0;i<4;i++)while(!atomic_load(&tasks[i].start))wait_value(&tasks[i].start,0);
    uint64_t begin=now();
    for(int i=0;i<4;i++){atomic_store(&tasks[i].go,1);wake(&tasks[i].go);}
    for(int i=0;i<4;i++){join(&tasks[i].tid);CHECK(!atomic_load(&tasks[i].error));}
    uint64_t elapsed=now()-begin;
    for(int i=0;i<4;i++)CHECK(munmap(tasks[i].stack,1<<20)==0);
    printf("{\"case\":\"worker-process-family\",\"threads\":4,\"rounds\":%d,\"elapsed_ns\":%llu,\"passed\":true}\n",rounds,(unsigned long long)elapsed);
    return 0;
}
