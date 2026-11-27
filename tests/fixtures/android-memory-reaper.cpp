// SPDX-License-Identifier: GPL-3.0-or-later
// Link with unchanged AOSP android-13.0.0_r83 Reaper.cpp and image libraries.
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/pidfd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "reaper.h"
static void check(bool b,const char*s){if(!b){perror(s);exit(1);}}
int main(){setvbuf(stdout,NULL,_IONBF,0);alarm(10);check(Reaper::is_reaping_supported(),"reaper supported");int ready[2],fail[2];check(!pipe(ready)&&!pipe(fail),"pipes");pid_t pid=fork();check(pid>=0,"fork");if(!pid){close(ready[0]);unsigned char*p=(unsigned char*)mmap(NULL,64*1024*1024,3,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);check(p!=MAP_FAILED,"child mmap");for(int a=0;a<64*1024*1024;a+=4096)p[a]=1;check(write(ready[1],"r",1)==1,"resident ready");for(;;)pause();}close(ready[1]);char c;check(read(ready[0],&c,1)==1,"read ready");int fd=pidfd_open(pid,0);check(fd>=0,"pidfd_open");Reaper*reaper=new Reaper;reaper->enable_debug(true);check(reaper->init(fail[1])&&reaper->thread_cnt()==2,"original reaper threads");check(!reaper->kill({fd,pid,getuid()},false),"original async kill");close(fd);int status;check(waitpid(pid,&status,0)==pid&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGKILL,"original reaper killed target");usleep(100000);puts("PASS original AOSP13 Reaper.cpp: async queue, duplicate pidfd, SIGKILL, process_mrelease, 64 MiB target");_exit(0);}
