// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
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

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL line=%d %s errno=%d\n", __LINE__, #x, errno); _exit(1); } } while (0)
static _Atomic int leader;
static int mode;
static int benchmark;
static uint64_t now(void) {
    struct timespec t;
    CHECK(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec;
}
static int last(void *unused) {
    (void)unused;
    struct timespec timeout = {1, 0};
    uint64_t end = now() + 5000000000ull;
    for (;;) {
        int tid = atomic_load(&leader);
        if (!tid) break;
        CHECK(now() < end);
        long r = syscall(SYS_futex, &leader, 0, tid, &timeout, 0, 0);
        CHECK(r == 0 || errno == EAGAIN || errno == EINTR || errno == ETIMEDOUT);
    }
    if (mode == 0) syscall(SYS_exit, benchmark ? 0 : 37);
    if (mode == 1) syscall(SYS_exit_group, 31);
    CHECK(kill(getpid(), mode == 2 ? SIGTERM : SIGKILL) == 0);
    for (;;) syscall(SYS_sched_yield);
}
int main(int argc, char **argv) {
    benchmark = argc > 1 && argv[1][0] == 'b';
    int rounds = argc > 1 + benchmark ? atoi(argv[1 + benchmark]) : 8;
    CHECK(rounds > 0 && rounds <= 10000);
    uint64_t begin = now();
    for (int r = 0; r < rounds; r++) {
        for (mode = 0; mode < (benchmark ? 1 : 4); mode++) {
            for (int owner_control = 0; owner_control < 2; owner_control++) {
                pid_t child = fork();
                CHECK(child >= 0);
                if (child == 0) {
                    if (owner_control) CHECK(epoll_create1(0) >= 0);
                    atomic_store(&leader, (int)syscall(SYS_gettid));
                    CHECK(syscall(SYS_set_tid_address, &leader) == atomic_load(&leader));
                    void *stack = mmap(0, 1 << 20, PROT_READ | PROT_WRITE,
                                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    CHECK(stack != MAP_FAILED);
                    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;
                    CHECK(clone(last, (char *)stack + (1 << 20), flags, 0) > 0);
                    syscall(SYS_exit, benchmark ? 0 : 11);
                    __builtin_unreachable();
                }
                int status = 0;
                CHECK(waitpid(child, &status, 0) == child);
                int expected = benchmark ? 0 : mode < 2 ? (mode ? 31 : 37) << 8 : (mode == 2 ? SIGTERM : SIGKILL);
                if (status != expected) fprintf(stderr, "round=%d mode=%d owner=%d pid=%d status=%d expected=%d\n",
                                                r, mode, owner_control, child, status, expected);
                CHECK(status == expected);
            }
        }
    }
    printf("{\"case\":\"worker-thread-completion\",\"groups\":%d,\"elapsed_ns\":%llu,\"passed\":true}\n",
           rounds * (benchmark ? 2 : 8), (unsigned long long)(now() - begin));
    return 0;
}
