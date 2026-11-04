// SPDX-License-Identifier: GPL-3.0-or-later
// Real cgroup v2 controls: original event FDs, live threads, migration and quota.
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct shared {
    _Atomic uint64_t count[3];
    _Atomic int ready, quota, quota_result, peer_stop;
};
static struct shared *state;
static pid_t worker = -1, peer = -1;
static void fail(const char *what, int line) {
    int error = errno;
    if (worker > 0) kill(worker, SIGKILL);
    if (peer > 0) kill(peer, SIGKILL);
    fprintf(stderr, "FAIL line=%d %s: %s\n", line, what, strerror(error));
    exit(1);
}
#define CHECK(expr) do { if (!(expr)) fail(#expr, __LINE__); } while (0)
static double now(void) {
    struct timespec ts;
    CHECK(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}
static void brief(void) {
    struct timespec ts = {0, 50000};
    nanosleep(&ts, NULL);
}
static int control(const char *base, const char *name, int flags) {
    char path[256];
    CHECK(snprintf(path, sizeof path, "%s/%s", base, name) < (int)sizeof path);
    int fd = open(path, flags | O_CLOEXEC);
    CHECK(fd >= 0);
    return fd;
}
static void put(int fd, const char *text) {
    size_t size = strlen(text);
    CHECK(write(fd, text, size) == (ssize_t)size);
}
static void get(int fd, char text[128]) {
    ssize_t count = pread(fd, text, 127, 0);
    CHECK(count >= 0);
    text[count] = 0;
}
static uint64_t counter(void) {
    return atomic_load_explicit(&state->count[0], memory_order_relaxed)
         + atomic_load_explicit(&state->count[1], memory_order_relaxed);
}
static void resumed(uint64_t old) {
    double deadline = now() + 5;
    while (counter() == old) {
        CHECK(now() < deadline);
        brief();
    }
}
static void frozen(int events, int epoll, int wanted) {
    double deadline = now() + 5;
    for (;;) {
        char text[128];
        get(events, text);
        CHECK(strlen(text) >= 20);
        if (text[19] == '0' + wanted) return;
        CHECK(now() < deadline);
        if (epoll < 0) { brief(); continue; }
        struct epoll_event event;
        int count = epoll_wait(epoll, &event, 1, 5000);
        CHECK(count == 1 && (event.events & (EPOLLPRI | EPOLLERR)));
    }
}
static void *busy(void *unused) {
    (void)unused;
    for (;;) atomic_fetch_add_explicit(&state->count[1], 1, memory_order_relaxed);
    return NULL;
}
int main(int argc, char **argv) {
    int rounds = argc > 1 ? atoi(argv[1]) : 8;
    int bench = argc > 2 && strcmp(argv[2], "bench") == 0;
    CHECK(rounds > 0 && rounds <= 10000);
    state = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    CHECK(state != MAP_FAILED);
    char base[128], work[160], inner[192];
    snprintf(base, sizeof base, "/sys/fs/cgroup/trames-worker-%ld", (long)getpid());
    snprintf(work, sizeof work, "%s/work", base);
    snprintf(inner, sizeof inner, "%s/inner", work);
    // Linux test hosts normally already delegate pids at the root; standalone
    // Trames starts with controllers disabled. This changes only that instance.
    int root_subtree = control("/sys/fs/cgroup", "cgroup.subtree_control", O_WRONLY);
    put(root_subtree, "+pids"); close(root_subtree);
    CHECK(mkdir(base, 0755) == 0);
    int subtree = control(base, "cgroup.subtree_control", O_WRONLY);
    put(subtree, "+pids");
    CHECK(mkdir(work, 0755) == 0);
    CHECK(mkdir(inner, 0755) == 0);
    int procs = control(work, "cgroup.procs", O_WRONLY);
    int freeze = control(work, "cgroup.freeze", O_WRONLY);
    int events = control(work, "cgroup.events", O_RDONLY);
    int current = control(work, "pids.current", O_RDONLY);
    int max = control(work, "pids.max", O_WRONLY);
    int pids = control(work, "pids.events", O_RDONLY);
    // The old runtime cannot register cgroup files with epoll. Timing uses
    // identical pread-based observation on both versions; compat tests epoll.
    int epoll = bench ? -1 : epoll_create1(EPOLL_CLOEXEC);
    CHECK(bench || epoll >= 0);
    struct epoll_event event = {.events = EPOLLPRI | EPOLLERR, .data.u64 = 1};
    CHECK(bench || epoll_ctl(epoll, EPOLL_CTL_ADD, events, &event) == 0);
    worker = fork();
    CHECK(worker >= 0);
    if (worker == 0) {
        pthread_t thread;
        CHECK(pthread_create(&thread, NULL, busy, NULL) == 0);
        atomic_store(&state->ready, 1);
        for (;;) {
            atomic_fetch_add_explicit(&state->count[0], 1, memory_order_relaxed);
            if (atomic_exchange(&state->quota, 0)) {
                pid_t child = fork();
                int error = errno;
                if (child == 0) _exit(0);
                if (child > 0) CHECK(waitpid(child, NULL, 0) == child);
                atomic_store(&state->quota_result, child < 0 ? error : -1);
            }
        }
    }
    double deadline = now() + 5;
    while (!atomic_load(&state->ready)) { CHECK(now() < deadline); brief(); }
    char text[128];
    snprintf(text, sizeof text, "%ld", (long)worker);
    put(procs, text);
    get(current, text);
    CHECK(atoi(text) == 2);
    peer = fork();
    CHECK(peer >= 0);
    if (peer == 0) {
        while (!atomic_load_explicit(&state->peer_stop, memory_order_relaxed))
            atomic_fetch_add_explicit(&state->count[2], 1, memory_order_relaxed);
        _exit(0);
    }
    double start = now();
    for (int i = 0; i < rounds; ++i) {
        put(freeze, "1");
        frozen(events, epoll, 1);
        uint64_t old = counter();
        get(current, text);
        CHECK(atoi(text) == 2 && counter() == old);
        put(freeze, "0");
        resumed(old);
    }
    double milliseconds = (now() - start) * 1000;
    if (!bench) {
        int inner_procs = control(inner, "cgroup.procs", O_WRONLY);
        int inner_freeze = control(inner, "cgroup.freeze", O_WRONLY);
        put(freeze, "1"); frozen(events, epoll, 1);
        put(inner_freeze, "1");
        snprintf(text, sizeof text, "%ld", (long)worker); put(inner_procs, text);
        uint64_t old = counter();
        put(freeze, "0"); frozen(events, epoll, 0);
        brief(); CHECK(counter() == old);
        snprintf(text, sizeof text, "%ld", (long)worker); put(procs, text);
        resumed(old);
        close(inner_procs); close(inner_freeze);
        get(pids, text);
        put(max, "2");
        atomic_store(&state->quota, 1);
        deadline = now() + 5;
        while (!atomic_load(&state->quota_result)) { CHECK(now() < deadline); brief(); }
        CHECK(atomic_load(&state->quota_result) == EAGAIN);
        struct pollfd pollfd = {.fd = pids, .events = POLLPRI};
        CHECK(poll(&pollfd, 1, 0) == 1 && (pollfd.revents & POLLPRI));
        get(pids, text); CHECK(strstr(text, "max 1\n"));
        put(freeze, "1"); frozen(events, epoll, 1);
    }
    int killfd = control(work, "cgroup.kill", O_WRONLY);
    put(killfd, "1");
    int status;
    CHECK(waitpid(worker, &status, 0) == worker);
    worker = -1;
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
    get(events, text); CHECK(strstr(text, "populated 0\n"));
    atomic_store(&state->peer_stop, 1);
    CHECK(waitpid(peer, &status, 0) == peer);
    peer = -1;
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(atomic_load(&state->count[2]) > 0);
    int fds[] = {subtree, procs, freeze, events, current, max, pids, epoll, killfd};
    for (size_t i = 0; i < sizeof fds / sizeof fds[0]; ++i)
        if (fds[i] >= 0) close(fds[i]);
    CHECK(rmdir(inner) == 0 && rmdir(work) == 0 && rmdir(base) == 0);
    printf("worker-cgroup %s PASS rounds=%d milliseconds=%.3f\n",
           bench ? "bench" : "compat", rounds, milliseconds);
    return 0;
}
