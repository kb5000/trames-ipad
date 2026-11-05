// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
extern char **environ;
#ifdef __APPLE__
#include <mach-o/dyld.h>
static char self_image[4096];
#define SELF_IMAGE self_image
#else
#define SELF_IMAGE "/proc/self/exe"
#endif
static uint64_t stamp(void) { struct timespec t;assert(clock_gettime(CLOCK_MONOTONIC,&t)==0);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec; }
static void wait_ok(pid_t pid) { int s;assert(waitpid(pid,&s,0)==pid && WIFEXITED(s) && WEXITSTATUS(s)==0); }
static void execute(const char *mode,int remaining,int pid) {
 char left[24], identity[24];snprintf(left,sizeof left,"%d",remaining);snprintf(identity,sizeof identity,"%d",pid);
 char *args[]={"worker-exec","chain",(char *)mode,left,identity,NULL};
#ifndef __APPLE__
 if(!strcmp(mode,"at")) { int fd=open(SELF_IMAGE,O_PATH|O_CLOEXEC);assert(fd>=0);syscall(SYS_execveat,fd,"",args,environ,AT_EMPTY_PATH); }
 else
#endif
 execve(SELF_IMAGE,args,environ);
 perror("exec");_exit(90);
}
static int thread_remaining,thread_pid;
static void *thread_exec(void *unused) { (void)unused;execute("thread",thread_remaining,thread_pid);return NULL; }
int main(int argc,char **argv) {
#ifdef __APPLE__
 uint32_t size=sizeof self_image;assert(_NSGetExecutablePath(self_image,&size)==0);
#endif
 if(argc==5 && !strcmp(argv[1],"chain")) {
  const char *mode=argv[2];int remaining=atoi(argv[3]),pid=atoi(argv[4]);
  assert(getpid()==pid);
#ifndef __APPLE__
  assert(syscall(SYS_gettid)==pid);
#endif
  if(!remaining) return 0;
  if(!strcmp(mode,"thread")) { pthread_t t;thread_remaining=remaining-1;thread_pid=pid;assert(pthread_create(&t,NULL,thread_exec,NULL)==0);for(;;) pause(); }
  execute(mode,remaining-1,pid);
 }
 const char *mode=argc>1?argv[1]:"plain";int rounds=argc>2?atoi(argv[2]):32;assert(rounds>0);
 _Atomic int *stop=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_SHARED,-1,0);assert(stop!=MAP_FAILED);
 pid_t busy=fork();assert(busy>=0);if(!busy) {while(!atomic_load_explicit(stop,memory_order_relaxed)){} _exit(0);}
 uint64_t began=stamp();pid_t child=fork();assert(child>=0);
 if(!child) {
  if(!strcmp(mode,"failed")) {
   int fd=open(SELF_IMAGE,O_RDONLY|O_CLOEXEC);assert(fd>=0);char *args[]={"missing",NULL};
   for(int i=0;i<rounds;i++){errno=0;assert(execve("/missing-exec-image",args,environ)==-1 && errno==ENOENT);assert(fcntl(fd,F_GETFD)==FD_CLOEXEC);}
   _exit(0);
  }
  execute(mode,rounds-1,getpid());
 }
 wait_ok(child);uint64_t elapsed=stamp()-began;atomic_store_explicit(stop,1,memory_order_relaxed);wait_ok(busy);assert(munmap(stop,4096)==0);
 printf("{\"case\":\"worker-exec\",\"mode\":\"%s\",\"rounds\":%d,\"elapsed_ns\":%llu,\"passed\":true}\n",mode,rounds,(unsigned long long)elapsed);return 0;
}
