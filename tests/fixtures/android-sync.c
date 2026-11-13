// SPDX-License-Identifier: GPL-3.0-or-later
// Same Linux fence UAPI on native ARM64 Linux and Android's original libsync.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <linux/sync_file.h>
#ifdef __ANDROID__
#include <dlfcn.h>
#endif
struct create_fence { uint32_t value; char name[32]; int32_t fence; };
#define CREATE _IOWR('W',0,struct create_fence)
#define INC _IOW('W',1,uint32_t)
static int (*original_wait)(int,int);
static int (*original_merge)(const char*,int,int);
static void check(int ok,const char *what){if(!ok){perror(what);exit(1);}}
static uint64_t now(void){struct timespec t;check(clock_gettime(CLOCK_MONOTONIC,&t)==0,"clock");return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static int point(int timeline,uint32_t value){struct create_fence p={.value=value};strcpy(p.name,"trames");check(ioctl(timeline,CREATE,&p)==0,"create fence");check(fcntl(p.fence,F_GETFD)&FD_CLOEXEC,"fence cloexec");return p.fence;}
static void increment(int timeline,uint32_t value){check(ioctl(timeline,INC,&value)==0,"timeline increment");}
static int combine(int a,int b){if(original_merge){int fd=original_merge("combined",a,b);check(fd>=0,"original sync_merge");return fd;}struct sync_merge_data p={.fd2=b};strcpy(p.name,"combined");check(ioctl(a,SYNC_IOC_MERGE,&p)==0,"merge");return p.fence;}
static int wait_fence(int fd,int ms){if(original_wait)return original_wait(fd,ms);struct pollfd p={.fd=fd,.events=POLLIN};int r=poll(&p,1,ms);if(r>0)return 0;if(r==0)errno=ETIME;return -1;}
static struct sync_file_info info(int fd,int entries,struct sync_fence_info *points){struct sync_file_info p={.num_fences=entries,.sync_fence_info=(uintptr_t)points};check(ioctl(fd,SYNC_IOC_FILE_INFO,&p)==0,"fence info");return p;}
int main(int argc,char **argv){
    const char *device=argc>1?argv[1]:"/dev/sw_sync";
#ifdef __ANDROID__
    void *library=dlopen("libsync.so",RTLD_NOW|RTLD_LOCAL);check(library!=NULL,"original libsync");
    original_wait=dlsym(library,"sync_wait");original_merge=dlsym(library,"sync_merge");check(original_wait&&original_merge,"libsync symbols");
#endif
    int a=open(device,O_RDWR|O_CLOEXEC),b=open(device,O_RDWR|O_CLOEXEC);check(a>=0&&b>=0,"timelines");
    int early=point(a,1),late=point(a,3),foreign=point(b,2),same=combine(early,late);
    check(info(same,0,NULL).num_fences==1,"same timeline merge coalesces");
    int both=combine(same,foreign);struct sync_fence_info detail[2];
    check(info(both,2,detail).num_fences==2,"independent timeline merge");
    errno=0;check(wait_fence(both,0)==-1&&errno==ETIME,"pending fence timeout");
    int ready[2];check(pipe(ready)==0,"pipe");pid_t child=fork();check(child>=0,"fork");
    if(!child){close(a);close(b);close(ready[0]);check(write(ready[1],"x",1)==1,"child ready");check(wait_fence(both,3000)==0,"original sync_wait wake");_exit(0);}
    close(ready[1]);char byte;check(read(ready[0],&byte,1)==1,"ready wait");close(ready[0]);
    uint64_t before=now();increment(a,3);check(wait_fence(early,0)==0,"early signaled");check(wait_fence(late,0)==0,"late signaled");
    check(wait_fence(both,0)==-1&&errno==ETIME,"merged requires all points");increment(b,2);
    int status;check(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0,"wait child");
    check(info(both,2,detail).status==1,"merged info signals");
    uint64_t after=now();for(int i=0;i<2;i++)check(detail[i].status==1&&detail[i].timestamp_ns>=before&&detail[i].timestamp_ns<=after,"guest monotonic timestamps");
    struct sync_file_info bad={.num_fences=1,.sync_fence_info=(uintptr_t)detail};errno=0;check(ioctl(both,SYNC_IOC_FILE_INFO,&bad)==-1&&errno==EINVAL,"short info array");
    int abandoned=point(a,4),alias=dup(a);check(alias>=0,"timeline dup");close(a);check(info(abandoned,0,NULL).status==0,"alias retains timeline");close(alias);check(wait_fence(abandoned,0)==0,"closed timeline signals error");check(info(abandoned,1,detail).status==-ENOENT,"closed timeline error");
    int wrap=open(device,O_RDWR|O_CLOEXEC);check(wrap>=0,"wrap timeline");increment(wrap,0x7fffffff);int almost=point(wrap,0xfffffff0);increment(wrap,0x7ffffff0);check(wait_fence(almost,0)==-1&&errno==ETIME,"pre-wrap pending");increment(wrap,1);check(wait_fence(almost,0)==0,"large increment");int wrapped=point(wrap,5);increment(wrap,21);check(wait_fence(wrapped,0)==0,"unsigned timeline wrap");
    void *readonly=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(readonly!=MAP_FAILED,"test mapping");memset(readonly,0,sizeof(struct create_fence));check(mprotect(readonly,4096,PROT_READ)==0,"readonly request");errno=0;check(ioctl(wrap,CREATE,readonly)==-1&&errno==EFAULT,"copyout rollback");munmap(readonly,4096);
    close(early);close(late);close(foreign);close(same);close(both);close(abandoned);close(almost);close(wrapped);close(wrap);close(b);
#ifdef __ANDROID__
    dlclose(library);puts("android-sync: PASS original libsync, merge, poll, close, fork, wrap, copyout and clock");
#else
    puts("android-sync: PASS native UAPI, merge, poll, close, fork, wrap, copyout and clock");
#endif
    return 0;
}
