// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

static char stack[65536] __attribute__((aligned(16)));
static volatile sig_atomic_t delivered;
static void handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    assert(sig == SIGUSR1);
    delivered = info->si_pid;
}
static void waited(pid_t pid) {
    int status;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
static void unrelated_birth(void) {
    pid_t child = fork();
    assert(child >= 0);
    if (!child) _exit(0);
    waited(child);
}
static void queued_sender(int mode) {
    sigset_t blocked, old, empty;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGUSR1);
    sigemptyset(&empty);
    struct sigaction action = {.sa_sigaction = handler, .sa_flags = SA_SIGINFO};
    sigemptyset(&action.sa_mask);
    assert(sigaction(SIGUSR1, &action, NULL) == 0);
    assert(sigprocmask(SIG_BLOCK, &blocked, &old) == 0);
    int fd = mode == 1 ? signalfd(-1, &blocked, SFD_CLOEXEC | SFD_NONBLOCK) : -1;
    assert(mode != 1 || fd >= 0);
    delivered = 0;
    pid_t sender = fork();
    assert(sender >= 0);
    if (!sender) {
        assert(kill(1, SIGUSR1) == 0);
        _exit(0);
    }
    waited(sender);
    // Reaping and another birth must not erase the queued sender's local PID.
    unrelated_birth();
    if (mode == 0) {
        siginfo_t info;
        assert(sigwaitinfo(&blocked, &info) == SIGUSR1);
        assert(info.si_pid == sender);
    } else if (mode == 1) {
        struct signalfd_siginfo info;
        assert(read(fd, &info, sizeof info) == sizeof info);
        assert(info.ssi_signo == SIGUSR1 && info.ssi_pid == (unsigned)sender);
        assert(close(fd) == 0);
    } else {
        while (!delivered) sigsuspend(&empty);
        assert(delivered == sender);
    }
    assert(sigprocmask(SIG_SETMASK, &old, NULL) == 0);
}
static void surviving_group(void) {
    int announced[2], release[2];
    assert(pipe(announced) == 0 && pipe(release) == 0);
    pid_t leader = fork();
    assert(leader >= 0);
    if (!leader) {
        assert(setsid() == getpid());
        pid_t member = fork();
        assert(member >= 0);
        if (member) _exit(0);
        pid_t own = getpid();
        assert(write(announced[1], &own, sizeof own) == sizeof own);
        char byte;
        assert(read(release[0], &byte, 1) == 1);
        assert(getppid() == 1);
        assert(setsid() == own && getsid(0) == own && getpgrp() == own);
        _exit(0);
    }
    pid_t member;
    assert(read(announced[0], &member, sizeof member) == sizeof member);
    waited(leader);
    unrelated_birth();
    assert(getpgid(member) == leader && getsid(member) == leader);
    assert(write(release[1], "x", 1) == 1);
    waited(member);
    assert(close(announced[0]) == 0 && close(announced[1]) == 0);
    assert(close(release[0]) == 0 && close(release[1]) == 0);
}
static int isolated(void *unused) {
    (void)unused;
    assert(getpid() == 1 && setsid() == 1);
    for (int mode = 0; mode < 3; ++mode) queued_sender(mode);
    surviving_group();
    return 0;
}
int main(void) {
    pid_t init = clone(isolated, stack + sizeof stack, CLONE_NEWPID | SIGCHLD, NULL);
    assert(init > 0);
    waited(init);
    puts("{\"case\":\"pid-identity\",\"passed\":true}");
    return 0;
}
