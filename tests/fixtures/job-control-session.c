// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static int master, slave, rounds;
static void status(pid_t child, int event, int value) {
    int s; assert(waitpid(child, &s, event) == child);
    if (event == WUNTRACED) assert(WIFSTOPPED(s) && WSTOPSIG(s) == value);
    else assert(WIFEXITED(s) && WEXITSTATUS(s) == value);
}
static void foreground(pid_t group) {
    sigset_t mask, old; sigemptyset(&mask); sigaddset(&mask, SIGTTOU);
    assert(!pthread_sigmask(SIG_BLOCK, &mask, &old));
    assert(!tcsetpgrp(slave, group));
    assert(!pthread_sigmask(SIG_SETMASK, &old, NULL));
}
static void background(int operation) {
    struct termios attr; assert(!tcgetattr(slave, &attr));
    if (operation == 1) attr.c_lflag |= TOSTOP;
    else attr.c_lflag &= ~TOSTOP;
    assert(!tcsetattr(slave, TCSANOW, &attr));
    pid_t child=fork(); assert(child>=0);
    if (!child) {
        close(master); assert(!setpgid(0,0));
        assert(!fcntl(slave,F_SETFL,O_NONBLOCK));
        if (operation == 0) { char b[8]; assert(read(slave,b,sizeof b)==3 && !memcmp(b,"ok\n",3)); }
        else if (operation == 1) assert(write(slave,"w",1)==1);
        else { struct termios t; assert(!tcgetattr(slave,&t)); if (tcsetattr(slave,TCSANOW,&t)) { perror("background tcsetattr"); abort(); } }
        _exit(23);
    }
    assert(!setpgid(child,child));
    int s; assert(waitpid(-child,&s,WUNTRACED)==child);
    assert(WIFSTOPPED(s) && WSTOPSIG(s)==(operation==0?SIGTTIN:SIGTTOU));
    foreground(child);
    if (!operation) assert(write(master,"ok\n",3)==3);
    assert(!kill(-child,SIGCONT)); status(child,0,23); foreground(getpid());
}
static void terminal_signal(int sig) {
    int sync[2]; assert(!pipe(sync));
    pid_t child=fork(); assert(child>=0);
    if (!child) {
        close(master); close(sync[0]); assert(!setpgid(0,0));
        sigset_t mask; sigemptyset(&mask); sigaddset(&mask,sig);
        assert(!pthread_sigmask(SIG_BLOCK,&mask,NULL));
        assert(write(sync[1],"r",1)==1);
        siginfo_t info; assert(sigwaitinfo(&mask,&info)==sig);
        _exit(24);
    }
    close(sync[1]); char b; assert(read(sync[0],&b,1)==1); close(sync[0]);
    foreground(child);
    if (sig==SIGINT) assert(write(master,"\003",1)==1);
    else { struct winsize size={.ws_row=41,.ws_col=121}; assert(!ioctl(slave,TIOCSWINSZ,&size)); }
    status(child,0,24); foreground(getpid());
}
static void *queries(void *unused) {
    (void)unused; pid_t sid=getsid(0), pgid=getpgid(0);
    for(int i=0;i<rounds;i++) {
        assert(getpgid(0)==pgid && getsid(0)==sid);
        assert(tcgetsid(slave)==sid && tcgetpgrp(slave)==pgid);
        struct termios attr; struct winsize window;
        assert(!tcgetattr(slave,&attr) && !ioctl(slave,TIOCGWINSZ,&window));
    }
    return NULL;
}
static void session(int query) {
    assert(setsid()==getpid());
    master=posix_openpt(O_RDWR|O_NOCTTY); assert(master>=0);
    assert(!grantpt(master) && !unlockpt(master));
    char path[128]; assert(!ptsname_r(master,path,sizeof path));
    slave=open(path,O_RDWR); assert(slave>=0);
    assert(tcgetsid(slave)==getpid() && tcgetpgrp(slave)==getpid());
    int tty=open("/dev/tty",O_RDWR); assert(tty>=0);close(tty);
    if (query) {
        pthread_t peers[3]; for (int i=0;i<3;i++) assert(!pthread_create(peers+i,NULL,queries,NULL));
        queries(NULL); for(int i=0;i<3;i++) assert(!pthread_join(peers[i],NULL));
        _exit(0);
    }
    background(0); background(1); background(2);
    terminal_signal(SIGINT); terminal_signal(SIGWINCH);
    pid_t child=fork(); assert(child>=0);
    if (!child) {
        close(master); assert(!ioctl(slave,TIOCNOTTY,0));
        errno=0;assert(open("/dev/tty",O_RDWR)==-1 && errno==ENXIO);_exit(25);
    }
    status(child,0,25);assert(tcgetsid(slave)==getpid());
    // Leader detachment clears the session; ignore the expected self HUP.
    signal(SIGHUP,SIG_IGN); assert(!ioctl(slave,TIOCNOTTY,0));
    errno=0;assert(open("/dev/tty",O_RDWR)==-1 && errno==ENXIO);
    close(slave);slave=open(path,O_RDWR);assert(slave>=0 && tcgetsid(slave)==getpid());
    close(master);
    errno=0;assert(open("/dev/tty",O_RDWR)==-1 && errno==ENXIO);
    close(slave);_exit(0);
}
int main(int argc,char **argv) {
    int query=argc>1 && !strcmp(argv[1],"queries"); rounds=argc>2?atoi(argv[2]):1000;
    assert(rounds>0);
    struct timespec a,b;assert(!clock_gettime(CLOCK_MONOTONIC,&a));
    pid_t child=fork();assert(child>=0);if(!child)session(query);status(child,0,0);
    assert(!clock_gettime(CLOCK_MONOTONIC,&b));
    uint64_t ns=(uint64_t)(b.tv_sec-a.tv_sec)*1000000000+b.tv_nsec-a.tv_nsec;
    if(query)printf("{\"mode\":\"queries\",\"threads\":4,\"rounds\":%d,\"elapsed_ns\":%llu}\n",rounds,(unsigned long long)ns);
    else puts("job-control-session: auto ctty, background nonblock read/write/ioctl, input/resize signals, detach and final hangup: ok");
    return 0;
}
