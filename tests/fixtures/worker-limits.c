// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s errno=%d\n",__LINE__,#x,errno);exit(1); } } while(0)
static pthread_barrier_t barrier;
static struct rlimit original;
static pid_t tids[32];
static double elapsed[32];
static unsigned threads, loops;
static const char *mode;
static double now(void) {struct timespec t;CHECK(clock_gettime(CLOCK_MONOTONIC,&t)==0);return t.tv_sec+t.tv_nsec/1e9;}
static void *worker(void *arg) {
 unsigned id=(unsigned)(uintptr_t)arg;tids[id]=(pid_t)syscall(SYS_gettid);
 pthread_barrier_wait(&barrier);
 struct rlimit value={128,original.rlim_max},out;
 double start=now();
 for(unsigned i=0;i<loops;i++) {
  if(!strcmp(mode,"query")) {CHECK(syscall(SYS_getrlimit,RLIMIT_NOFILE,&out)==0);CHECK(out.rlim_cur==128);}
  else if(!strcmp(mode,"set")) CHECK(syscall(SYS_setrlimit,RLIMIT_NOFILE,&value)==0);
  else if(!strcmp(mode,"target")) {CHECK(syscall(SYS_prlimit64,tids[(id+1)%threads],RLIMIT_NOFILE,NULL,&out)==0);CHECK(out.rlim_cur==128);}
  else CHECK(0);
 }
 elapsed[id]=now()-start;pthread_barrier_wait(&barrier);return NULL;
}
static _Atomic unsigned size_signals, size_info_invalid;
static void size_signal(int sig, siginfo_t *info, void *context) {(void)sig;(void)context; if(info->si_code!=SI_USER || info->si_pid!=getpid() || info->si_uid!=getuid()) atomic_store_explicit(&size_info_invalid,1,memory_order_relaxed);atomic_fetch_add_explicit(&size_signals,1,memory_order_relaxed);}
static int size_fd;
static void *sized_writer(void *unused) {
 (void)unused;pthread_barrier_wait(&barrier);pthread_barrier_wait(&barrier);
 CHECK(pwrite(size_fd,"abcdefgh",8,4)==3);
 CHECK(pwrite(size_fd,"x",1,7)==-1 && errno==EFBIG);
 return NULL;
}
static void compatibility(void) {
 struct rlimit size,limited;CHECK(getrlimit(RLIMIT_FSIZE,&size)==0);
 struct sigaction action={0};action.sa_sigaction=size_signal;action.sa_flags=SA_SIGINFO;sigemptyset(&action.sa_mask);CHECK(sigaction(SIGXFSZ,&action,NULL)==0);
 size_fd=memfd_create("limits",0);CHECK(size_fd>=0);CHECK(write(size_fd,"abcd",4)==4);
 CHECK(pthread_barrier_init(&barrier,NULL,2)==0);pthread_t thread;CHECK(pthread_create(&thread,NULL,sized_writer,NULL)==0);
 pthread_barrier_wait(&barrier);limited=size;limited.rlim_cur=7;CHECK(setrlimit(RLIMIT_FSIZE,&limited)==0);pthread_barrier_wait(&barrier);
 CHECK(pthread_join(thread,NULL)==0);CHECK(atomic_load_explicit(&size_signals,memory_order_relaxed)==1);CHECK(!atomic_load_explicit(&size_info_invalid,memory_order_relaxed));
 CHECK(setrlimit(RLIMIT_FSIZE,&size)==0);CHECK(pwrite(size_fd,"z",1,7)==1);CHECK(close(size_fd)==0);CHECK(pthread_barrier_destroy(&barrier)==0);
 pid_t child=fork();CHECK(child>=0);
 if(!child) {
  struct rlimit lower={8,original.rlim_max};CHECK(setrlimit(RLIMIT_NOFILE,&lower)==0);
  int fds[16],n=0;while(n<16 && (fds[n]=open("/dev/null",O_RDONLY))>=0){CHECK(fds[n]<8);n++;}
  CHECK(errno==EMFILE && n>0 && n<16);while(n)CHECK(close(fds[--n])==0);_exit(0);
 }
 int status;CHECK(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
 struct rlimit after;CHECK(getrlimit(RLIMIT_NOFILE,&after)==0 && after.rlim_cur==128);
 CHECK(prlimit(child,RLIMIT_NOFILE,NULL,&after)==-1 && errno==ESRCH);
 puts("worker-limits-compatibility-ok");
}
int main(int argc,char **argv) {
 mode=argc>1?argv[1]:"compat";threads=argc>2?(unsigned)atoi(argv[2]):4;loops=argc>3?(unsigned)atoi(argv[3]):1000;
 CHECK(threads>0 && threads<=32 && loops>0);CHECK(getrlimit(RLIMIT_NOFILE,&original)==0 && original.rlim_max>=128);
 struct rlimit limit={128,original.rlim_max};CHECK(setrlimit(RLIMIT_NOFILE,&limit)==0);
 if(!strcmp(mode,"compat")) compatibility();
 else {
  CHECK(pthread_barrier_init(&barrier,NULL,threads)==0);pthread_t ids[32];
  for(unsigned n=0;n<threads;n++) CHECK(pthread_create(&ids[n],NULL,worker,(void*)(uintptr_t)n)==0);
  double maximum=0;for(unsigned n=0;n<threads;n++) {CHECK(pthread_join(ids[n],NULL)==0);if(elapsed[n]>maximum) maximum=elapsed[n];}
  printf("{\"mode\":\"%s\",\"threads\":%u,\"loops\":%u,\"calls\":%u,\"seconds\":%.9f}\n",mode,threads,loops,threads*loops,maximum);
  CHECK(pthread_barrier_destroy(&barrier)==0);
 }
 CHECK(setrlimit(RLIMIT_NOFILE,&original)==0);return 0;
}
