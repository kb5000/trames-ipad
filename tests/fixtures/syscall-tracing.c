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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sched.h>
#endif
#if defined(__APPLE__)
#include <sys/syscall.h>
#endif

enum { THREADS = 4 };
static unsigned rounds;
static int mixed, channel[2], restart_read;
static pid_t identity;
static atomic_int failures, entered, signals_seen;
static uint64_t now_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) _exit(90);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
static void check(int condition) {
    if (!condition) atomic_fetch_add(&failures, 1);
}
static long marked_pid(uint64_t tag) {
#if defined(__linux__) && defined(__aarch64__)
    register uint64_t x0 __asm__("x0") = tag;
    register uint64_t x1 __asm__("x1") = 0x1122;
    register uint64_t x2 __asm__("x2") = 0x3344;
    register uint64_t x3 __asm__("x3") = 0x5566;
    register uint64_t x4 __asm__("x4") = 0x7788;
    register uint64_t x5 __asm__("x5") = 0x99aa;
    register uint64_t x8 __asm__("x8") = 172;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3),
                     "r"(x4), "r"(x5), "r"(x8) : "memory", "cc");
    return (long)x0;
#elif defined(__APPLE__)
    (void)tag;
    // Darwin's getpid() is pure/cached and lets clang hoist this entire loop.
    // Use its syscall entry for an actual native kernel-crossing reference.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return syscall(SYS_getpid);
#pragma clang diagnostic pop
#else
    (void)tag;
    return (long)getpid();
#endif
}
static void *worker(void *opaque) {
    unsigned id = (unsigned)(uintptr_t)opaque;
    for (unsigned i = 0; i < rounds; ++i)
        check(marked_pid(0x5a000000ULL + ((uint64_t)id << 20) + i) == identity);
    if (mixed) {
#if defined(__linux__)
        uid_t real, effective, saved;
        check(getresuid(&real, &effective, &saved) == 0);
        check(marked_pid(0x6a000000ULL + id) == identity);
        check(unshare(0) == 0);
        check(marked_pid(0x6b000000ULL + id) == identity);
#endif
        long size = sysconf(_SC_PAGESIZE);
        void *memory = mmap(NULL, (size_t)size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        check(memory != MAP_FAILED);
        if (memory != MAP_FAILED) {
            memset(memory, 0xa5, (size_t)size);
            check(mprotect(memory, (size_t)size, PROT_READ) == 0);
            check(*(volatile unsigned char *)memory == 0xa5);
            check(munmap(memory, (size_t)size) == 0);
        }
        int sockets[2];
        check(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        check(write(sockets[0], "ok", 2) == 2);
        char bytes[3] = {0};
        check(read(sockets[1], bytes, 2) == 2 && !strcmp(bytes, "ok"));
        close(sockets[0]);
        close(sockets[1]);
        char path[128];
        snprintf(path, sizeof(path), "/tmp/trames-trace-%ld-%u", (long)identity, id);
        int fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0600);
        check(fd >= 0);
        check(write(fd, "file", 4) == 4 && lseek(fd, 0, SEEK_SET) == 0);
        char content[4];
        check(read(fd, content, sizeof(content)) == 4 && !memcmp(content, "file", 4));
        struct stat info;
        check(fstat(fd, &info) == 0 && info.st_size == 4);
        close(fd);
        check(unlink(path) == 0);
    }
    return NULL;
}
static void handler(int signal) {
    (void)signal;
    check(marked_pid(0x7e000001ULL + (unsigned)atomic_load(&signals_seen)) == identity);
    atomic_fetch_add(&signals_seen, 1);
}
static void *reader(void *unused) {
    (void)unused;
    atomic_store(&entered, 1);
    char byte = 0;
    ssize_t n = read(channel[0], &byte, 1);
    if (!restart_read) {
        check(n == -1 && errno == EINTR);
        n = read(channel[0], &byte, 1);
    }
    check(n == 1 && byte == 'q');
    return NULL;
}
static void pause_ms(long ms) {
    struct timespec delay = { .tv_sec = ms / 1000, .tv_nsec = ms % 1000 * 1000000 };
    while (nanosleep(&delay, &delay) && errno == EINTR) {}
}
static void signal_round(int restart) {
    restart_read = restart;
    atomic_store(&entered, 0);
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    action.sa_flags = restart ? SA_RESTART : 0;
    sigemptyset(&action.sa_mask);
    check(sigaction(SIGUSR1, &action, NULL) == 0);
    check(pipe(channel) == 0);
    pthread_t thread;
    check(pthread_create(&thread, NULL, reader, NULL) == 0);
    uint64_t deadline = now_ns() + 5000000000ULL;
    while (!atomic_load(&entered)) {
        if (now_ns() > deadline) _exit(91);
        pause_ms(1);
    }
    pause_ms(25);
    int before = atomic_load(&signals_seen);
    check(pthread_kill(thread, SIGUSR1) == 0);
    while (atomic_load(&signals_seen) == before) {
        if (now_ns() > deadline) _exit(92);
        pause_ms(1);
    }
    check(write(channel[1], "q", 1) == 1);
    check(pthread_join(thread, NULL) == 0);
    close(channel[0]);
    close(channel[1]);
}
int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "child")) return 9;
    mixed = argc > 1 && !strcmp(argv[1], "mixed");
    rounds = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 20;
    identity = getpid();
    (void)mkdir("/tmp", 0777);
    uint64_t began = now_ns();
    pthread_t threads[THREADS];
    for (unsigned i = 0; i < THREADS; ++i)
        if (pthread_create(&threads[i], NULL, worker, (void *)(uintptr_t)i)) return 93;
    for (unsigned i = 0; i < THREADS; ++i) check(pthread_join(threads[i], NULL) == 0);
    if (mixed) {
        signal_round(1);
        signal_round(0);
        pid_t child = fork();
        if (child == 0) {
            execl(argv[0], argv[0], "child", (char *)NULL);
            _exit(94);
        }
        int status = 0;
        check(child > 0 && waitpid(child, &status, 0) == child &&
              WIFEXITED(status) && WEXITSTATUS(status) == 9);
    }
    printf("{\"mode\":\"%s\",\"threads\":%d,\"rounds\":%u,\"pid\":%ld,\"signals\":%d,\"failure\":%d,\"elapsed_ns\":%llu}\n",
           mixed ? "mixed" : "queries", THREADS, rounds, (long)identity,
           atomic_load(&signals_seen), atomic_load(&failures), (unsigned long long)(now_ns() - began));
    return atomic_load(&failures) != 0;
}
