// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
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

struct shared {
    _Atomic unsigned ready, quit, tid, handled;
    _Atomic uint64_t counter[3];
};
static struct shared *s;
static void continued(int sig) {
    assert(sig == SIGCONT);
    atomic_fetch_add_explicit(&s->handled, 1, memory_order_relaxed);
}
static void *spin(void *arg) {
    uintptr_t id = (uintptr_t)arg;
    if (id == 1) atomic_store(&s->tid, syscall(SYS_gettid));
    atomic_fetch_add(&s->ready, 1);
    while (!atomic_load_explicit(&s->quit, memory_order_relaxed))
        atomic_fetch_add_explicit(&s->counter[id], 1, memory_order_relaxed);
    return NULL;
}
static uint64_t ns(void) {
    struct timespec t;
    assert(!clock_gettime(CLOCK_MONOTONIC, &t));
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static void send_control(pid_t child, int api, int sig) {
    int rc;
    if (api == 0) rc = kill(child, sig);
    else if (api == 1) rc = syscall(SYS_tkill, atomic_load(&s->tid), sig);
    else rc = syscall(SYS_tgkill, child, atomic_load(&s->tid), sig);
    assert(!rc);
}
int main(int argc, char **argv) {
    assert(argc == 4);
    int compat = !strcmp(argv[1], "compat");
    assert(compat || !strcmp(argv[1], "loops"));
    int mode = atoi(argv[2]), rounds = atoi(argv[3]);
    assert(mode >= 0 && mode <= 2 && rounds > 0);
    s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    assert(s != MAP_FAILED);
    pid_t child = fork();
    assert(child >= 0);
    if (!child) {
        assert(!setpgid(0, 0)); // non-orphaned job-control group in the parent's session
        struct sigaction sa = {.sa_handler = mode == 0 ? SIG_IGN : continued};
        assert(!sigaction(SIGCONT, &sa, NULL));
        if (mode == 1) {
            sigset_t mask;
            sigemptyset(&mask); sigaddset(&mask, SIGCONT);
            assert(!pthread_sigmask(SIG_BLOCK, &mask, NULL));
        }
        pthread_t thread;
        assert(!pthread_create(&thread, NULL, spin, (void *)1));
        spin(NULL);
        assert(!pthread_join(thread, NULL));
        if (mode == 1) {
            sigset_t pending, mask;
            assert(!sigpending(&pending) && sigismember(&pending, SIGCONT));
            sigemptyset(&mask); sigaddset(&mask, SIGCONT);
            assert(!pthread_sigmask(SIG_UNBLOCK, &mask, NULL));
        }
        if (mode != 0) assert(atomic_load(&s->handled) > 0);
        _exit(23);
    }
    pid_t peer = fork();
    assert(peer >= 0);
    if (!peer) { spin((void *)2); _exit(0); }
    while (atomic_load(&s->ready) != 3) usleep(1000);
    uint64_t began = ns();
    for (int i = 0; i < rounds; ++i) {
        int status, stop = compat ? (int[]){SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}[i % 4] : SIGSTOP;
        send_control(child, i % 3, stop);
        assert(waitpid(child, &status, WUNTRACED) == child);
        assert(WIFSTOPPED(status) && WSTOPSIG(status) == stop);
        if (compat) {
            uint64_t a = atomic_load(&s->counter[0]), b = atomic_load(&s->counter[1]);
            usleep(2000);
            assert(atomic_load(&s->counter[0]) == a && atomic_load(&s->counter[1]) == b);
        }
        send_control(child, mode == 1 ? 0 : (i + 1) % 3, SIGCONT);
        assert(waitpid(child, &status, WCONTINUED) == child && WIFCONTINUED(status));
    }
    uint64_t elapsed = ns() - began;
    atomic_store(&s->quit, 1);
    int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 23);
    assert(waitpid(peer, &status, 0) == peer && WIFEXITED(status) && !WEXITSTATUS(status));
    assert(atomic_load(&s->counter[2]) > 0);
    printf("{\"mode\":\"%s\",\"cont_policy\":%d,\"rounds\":%d,\"elapsed_ns\":%llu}\n",
        compat ? "compat" : "loops", mode, rounds, (unsigned long long)elapsed);
    return 0;
}
