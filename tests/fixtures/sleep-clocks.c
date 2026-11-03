// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
static uint64_t stamp(int clock) {struct timespec t;assert(clock_gettime(clock,&t)==0);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
int main(int argc,char **argv) {
    if(argc==3&&!strcmp(argv[1],"child")){assert(stamp(CLOCK_BOOTTIME)>=strtoull(argv[2],NULL,10));puts("sleep-clocks exec-origin-preserved");return 0;}
    uint64_t mono=stamp(CLOCK_MONOTONIC),boot=stamp(CLOCK_BOOTTIME);
    assert(boot+100000000>=mono); /* Native Linux may already have slept. */
    struct timespec raw;assert(syscall(SYS_clock_gettime,CLOCK_BOOTTIME,&raw)==0);
    uint64_t direct=(uint64_t)raw.tv_sec*1000000000+raw.tv_nsec;assert(direct>=boot&&direct-boot<100000000);
    int fd=timerfd_create(CLOCK_BOOTTIME,TFD_CLOEXEC);assert(fd>=0);
    struct itimerspec timer={0};uint64_t deadline=stamp(CLOCK_BOOTTIME)+20000000;
    timer.it_value.tv_sec=deadline/1000000000;timer.it_value.tv_nsec=deadline%1000000000;
    assert(timerfd_settime(fd,TFD_TIMER_ABSTIME,&timer,NULL)==0);
    struct pollfd pollfd={.fd=fd,.events=POLLIN};assert(poll(&pollfd,1,2000)==1);
    uint64_t count;assert(read(fd,&count,8)==8&&count==1);assert(stamp(CLOCK_BOOTTIME)>=deadline);
    timer.it_interval.tv_sec=0;timer.it_interval.tv_nsec=2000000;timer.it_value.tv_sec=0;timer.it_value.tv_nsec=2000000;
    assert(timerfd_settime(fd,0,&timer,NULL)==0);struct timespec delay={.tv_sec=0,.tv_nsec=15000000};assert(nanosleep(&delay,NULL)==0);
    assert(read(fd,&count,8)==8&&count>=2);assert(close(fd)==0);
    char expected[32];snprintf(expected,sizeof(expected),"%llu",(unsigned long long)stamp(CLOCK_BOOTTIME));
    int child=fork();assert(child>=0);if(!child){execl(argv[0],argv[0],"child",expected,(char *)NULL);_exit(127);}
    int status;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
    puts("sleep-clocks boottime-absolute-periodic-fork-exec-ok");return 0;
}
