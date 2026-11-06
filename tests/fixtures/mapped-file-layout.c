// SPDX-License-Identifier: GPL-3.0-or-later
// Busy mapped aliases must yield only their original memory domains to resize.
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

struct shared {
    _Atomic unsigned ready, stop;
    uint64_t reads[16];
};
static void check(int ok, const char *what) {
    if (!ok) { perror(what); exit(1); }
}
static uint64_t now(void) {
    struct timespec t;
    check(!clock_gettime(CLOCK_MONOTONIC, &t), "clock");
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
int main(int argc, char **argv) {
    unsigned count = argc > 1 ? (unsigned)strtoul(argv[1], NULL, 10) : 4;
    unsigned rounds = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 100;
    check(count && count <= 16 && rounds && rounds <= 100000, "arguments");
    char name[] = "mapped-layout-XXXXXX";
    int fd = mkstemp(name);
    check(fd >= 0 && !unlink(name) && !ftruncate(fd, 8192), "file");
    unsigned char marker = 0x75;
    check(pwrite(fd, &marker, 1, 0) == 1, "marker");
    struct shared *state = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANON, -1, 0);
    check(state != MAP_FAILED, "shared state");
    volatile unsigned char *mapped = mmap(NULL, 8192, PROT_READ,
                                         MAP_SHARED, fd, 0);
    check(mapped != MAP_FAILED, "original mapping");
    pid_t children[16];
    for (unsigned i = 0; i < count; i++) {
        children[i] = fork();
        check(children[i] >= 0, "fork");
        if (!children[i]) {
            check(mapped[0] == marker, "initial alias");
            uint64_t reads = 1;
            atomic_fetch_add_explicit(&state->ready, 1, memory_order_release);
            while (!atomic_load_explicit(&state->stop, memory_order_acquire)) {
                for (unsigned j = 0; j < 1024; j++)
                    check(mapped[0] == marker, "retained alias contents");
                reads += 1024;
                // Exercise new aliases while the original mapping is busy.
                if ((reads & 65535) == 1) {
                    volatile unsigned char *alias = mmap(NULL, 4096, PROT_READ,
                                                        MAP_SHARED, fd, 0);
                    check(alias != MAP_FAILED && alias[0] == marker, "new alias");
                    check(!munmap((void *)alias, 4096), "unmap alias");
                }
            }
            state->reads[i] = reads;
            _exit(0);
        }
    }
    while (atomic_load_explicit(&state->ready, memory_order_acquire) != count)
        sched_yield();
    uint64_t began = now();
    for (unsigned i = 0; i < rounds; i++) {
        check(!ftruncate(fd, i & 1 ? 8192 : 16384), "resize busy mappings");
    }
    uint64_t elapsed = now() - began;
    atomic_store_explicit(&state->stop, 1, memory_order_release);
    uint64_t total = 0;
    for (unsigned i = 0; i < count; i++) {
        int status;
        check(waitpid(children[i], &status, 0) == children[i]
              && WIFEXITED(status) && !WEXITSTATUS(status), "child completion");
        total += state->reads[i];
    }
    check(total && mapped[0] == marker, "reader progress");
    check(!munmap((void *)mapped, 8192) && !munmap(state, 4096) && !close(fd), "cleanup");
    printf("{\"passed\":true,\"readers\":%u,\"rounds\":%u,\"reads\":%llu,\"resize_ns\":%llu}\n",
           count, rounds, (unsigned long long)total, (unsigned long long)elapsed);
    return 0;
}
