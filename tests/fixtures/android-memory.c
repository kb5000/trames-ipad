// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <ucontext.h>
#include <unistd.h>
static void check(int okay,const char *step){if(!okay){perror(step);exit(1);}}
static volatile sig_atomic_t caught;
static void zygote_receive(void){
    int pair[2];check(!socketpair(AF_UNIX,SOCK_STREAM,0,pair),"zygote socketpair");
    int original=memfd_create("zygote-fd",MFD_CLOEXEC);check(original>=0,"zygote memfd");
    char byte='Z',control[CMSG_SPACE(sizeof(int))]={0};struct iovec iov={&byte,1};
    struct msghdr message={.msg_iov=&iov,.msg_iovlen=1,.msg_control=control,.msg_controllen=sizeof(control)};
    struct cmsghdr *c=CMSG_FIRSTHDR(&message);c->cmsg_level=SOL_SOCKET;c->cmsg_type=SCM_RIGHTS;c->cmsg_len=CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(c),&original,sizeof(int));check(sendmsg(pair[0],&message,MSG_NOSIGNAL)==1,"zygote send fd");
    memset(control,0,sizeof(control));byte=0;
    check(recvmsg(pair[1],&message,MSG_TRUNC|MSG_CTRUNC|MSG_CMSG_CLOEXEC|MSG_NOSIGNAL)==1&&byte=='Z',"Android ReceiveFileDescriptorVector flags");
    check(!(message.msg_flags&(MSG_CTRUNC|MSG_TRUNC)),"zygote complete control");
    c=CMSG_FIRSTHDR(&message);check(c&&c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_RIGHTS,"zygote received rights");
    int received;memcpy(&received,CMSG_DATA(c),sizeof(int));check(fcntl(received,F_GETFD)&FD_CLOEXEC,"zygote close-on-exec");
    close(received);close(original);close(pair[0]);close(pair[1]);
}
static void queued_signal(void){
    sigset_t set,old;sigemptyset(&set);sigaddset(&set,SIGUSR2);
    check(!sigprocmask(SIG_BLOCK,&set,&old),"block queued signal");
    siginfo_t info={0};info.si_code=SI_QUEUE;info.si_pid=getpid();info.si_uid=getuid();
    info.si_value.sival_ptr=(void*)0x12345678abcdef01UL;
    struct timespec timeout={0};
    for(int thread=0;thread<2;thread++){
        check(!syscall(thread?SYS_rt_tgsigqueueinfo:SYS_rt_sigqueueinfo,
            getpid(),thread?syscall(SYS_gettid):SIGUSR2,
            thread?(long)SIGUSR2:(long)&info,thread?(long)&info:0),"Android debugger queued signal");
        siginfo_t out={0};check(sigtimedwait(&set,&out,&timeout)==SIGUSR2,"receive queued signal");
        check(out.si_code==SI_QUEUE&&out.si_pid==getpid()&&out.si_uid==getuid()&&out.si_value.sival_ptr==info.si_value.sival_ptr,"queued signal payload");
    }
    check(!sigprocmask(SIG_SETMASK,&old,NULL),"restore signal mask");
}
static void fault(int sig,siginfo_t *info,void *context){
    if(sig!=SIGSEGV||info->si_addr)_exit(9);
    caught++;
    ((ucontext_t*)context)->uc_mcontext.pc+=4;
}
int main(void){
    setbuf(stdout,NULL);
    struct sigaction action={.sa_sigaction=fault,.sa_flags=SA_SIGINFO},old;
    check(!sigaction(SIGSEGV,&action,&old),"sigaction");
    struct {unsigned long handler,flags,restorer,mask;} probe={0},queried={0};
    check(!syscall(SYS_rt_sigaction,SIGSEGV,NULL,&probe,8),"query handler");
    probe.flags|=0x600; // SA_UNSUPPORTED plus an unknown bit.
    check(!syscall(SYS_rt_sigaction,SIGSEGV,&probe,NULL,8),"unsupported flag probe");
    check(!syscall(SYS_rt_sigaction,SIGSEGV,NULL,&queried,8),"query probed handler");
    check(!(queried.flags&0x600)&&(queried.flags&SA_SIGINFO),"reported supported flags");
    __asm__ volatile("mov x9, #0\nldr x9, [x9]":::"x9","memory");
    check(caught==1,"original synchronous signal context");
    check(!sigaction(SIGSEGV,&old,NULL),"restore handler");
    char destination[8]={0};char *pages=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
    check(pages!=MAP_FAILED,"mmap");memcpy(pages+4092,"abcd",4);check(!munmap(pages+4096,4096),"remote gap");
    struct iovec local={destination,8},remote={pages+4092,8};
    check(process_vm_readv(getpid(),&local,1,&remote,1,0)==4&&!memcmp(destination,"abcd",4),"ART safe read and partial fault");
    remote.iov_len=4;memcpy(destination,"wxyz",4);
    check(process_vm_writev(getpid(),&local,1,&remote,1,0)==4&&!memcmp(pages+4092,"wxyz",4),"same-mm write");
    check(!munmap(pages,4096),"unmap");
    int pipefd[2];check(!pipe(pipefd),"pipe");
    int size=fcntl(pipefd[0],F_SETPIPE_SZ,1024*1024);check(size>=1024*1024,"debugger pipe capacity");
    check(fcntl(pipefd[1],F_GETPIPE_SZ)==size,"shared pipe capacity");close(pipefd[0]);close(pipefd[1]);
    zygote_receive();queued_signal();
    puts("android-memory: PASS signal probing/context, ART safe read, partial faults, pipe capacity, Zygote fd receive and debugger queued signals");return 0;
}
