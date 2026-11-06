// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno); exit(1); } } while (0)
struct filter_instruction { uint16_t code; uint8_t yes, no; uint32_t value; };
struct filter_program { uint16_t count; struct filter_instruction *instructions; };
static pthread_barrier_t barrier;
static unsigned threads, loops;
static uint64_t durations[12];
static uint64_t now(void) {
    struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static void wait_barrier(void) {
    int result = pthread_barrier_wait(&barrier);
    CHECK(result == 0 || result == PTHREAD_BARRIER_SERIAL_THREAD);
}
static ssize_t body(const char *path, char *bytes, size_t capacity) {
    int fd = open(path, O_RDONLY | O_CLOEXEC); CHECK(fd >= 0);
    ssize_t n = read(fd, bytes, capacity); CHECK(n >= 0); CHECK(close(fd) == 0); return n;
}
static void *worker(void *arg) {
    unsigned id = (unsigned)(uintptr_t)arg;
    char path[128], bytes[8192]; struct stat st;
    snprintf(path, sizeof(path), "/proc/self/task/%ld", syscall(SYS_gettid));
    int directory = open(path, O_RDONLY | O_DIRECTORY); CHECK(directory >= 0);
    wait_barrier(); uint64_t began = now();
    for (unsigned n = 0; n < loops; n++) {
        int fd = openat(directory, "status", O_RDONLY); CHECK(fd >= 0);
        ssize_t got = pread(fd, bytes, sizeof(bytes), 0);
        CHECK(got > 0 && memmem(bytes, (size_t)got, "Name:", 5));
        CHECK(fstat(fd, &st) == 0 && S_ISREG(st.st_mode)); CHECK(close(fd) == 0);
        CHECK(fstatat(directory, "cwd", &st, 0) == 0 && S_ISDIR(st.st_mode));
        CHECK(readlinkat(directory, "exe", bytes, sizeof(bytes)) > 0);
    }
    durations[id] = now() - began; wait_barrier(); CHECK(close(directory) == 0); return NULL;
}
static void compatibility(char **argv) {
    char bytes[16384]; struct stat original, reopened;
    const char *paths[] = {"/proc/self/status", "/proc/self/stat", "/proc/self/statm",
        "/proc/self/maps", "/proc/self/auxv", "/proc/self/mountinfo", "/proc/meminfo",
        "/proc/stat", "/proc/cpuinfo", "/proc/uptime"};
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) CHECK(body(paths[i], bytes, sizeof(bytes)) > 0);
    int exe = open("/proc/self/exe", O_RDONLY); CHECK(exe >= 0);
    CHECK(fstat(exe, &original) == 0); char path[128];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", exe);
    int again = open(path, O_RDONLY); CHECK(again >= 0);
    CHECK(fstat(again, &reopened) == 0 && original.st_ino == reopened.st_ino);
    CHECK(read(exe, bytes, 1) == 1 && (unsigned char)bytes[0] == 0x7f);
    CHECK(read(again, bytes, 1) == 1 && (unsigned char)bytes[0] == 0x7f);
    CHECK(close(exe) == 0 && close(again) == 0);
    int ns = open("/proc/self/ns/mnt", O_RDONLY); CHECK(ns >= 0);
    snprintf(path, sizeof(path), "/proc/self/fd/%d", ns);
    again = open(path, O_RDONLY); CHECK(again >= 0);
    CHECK(fstat(ns, &original) == 0 && fstat(again, &reopened) == 0 && original.st_ino == reopened.st_ino);
    CHECK(close(ns) == 0 && close(again) == 0);
    char saved = argv[0][0]; argv[0][0] = 'Q';
    CHECK(body("/proc/self/cmdline", bytes, sizeof(bytes)) > 0 && bytes[0] == 'Q'); argv[0][0] = saved;
    char *payload = getenv("PATH"); CHECK(payload && strlen(payload) >= 8);
    int ready[2], release[2]; CHECK(pipe(ready) == 0 && pipe(release) == 0);
    pid_t child = fork(); CHECK(child >= 0);
    if (!child) {
        CHECK(close(ready[0]) == 0 && close(release[1]) == 0);
        argv[0][0] = 'Z'; payload[0] = 'Y';
        CHECK(write(ready[1], "r", 1) == 1 && read(release[0], bytes, 1) == 1); _exit(0);
    }
    CHECK(close(ready[1]) == 0 && close(release[0]) == 0);
    CHECK(read(ready[0], bytes, 1) == 1);
    snprintf(path, sizeof(path), "/proc/%d/cmdline", child);
    CHECK(body(path, bytes, sizeof(bytes)) > 0 && bytes[0] == 'Z');
    snprintf(path, sizeof(path), "/proc/%d/environ", child);
    ssize_t n = body(path, bytes, sizeof(bytes)); CHECK(n > 0 && memmem(bytes, (size_t)n, "PATH=Y", 6));
    CHECK(argv[0][0] == saved && payload[0] != 'Y');
    CHECK(write(release[1], "r", 1) == 1); int status;
    CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(close(ready[0]) == 0 && close(release[1]) == 0);
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "compat";
    threads = argc > 2 ? (unsigned)atoi(argv[2]) : 2;
    loops = argc > 3 ? (unsigned)atoi(argv[3]) : 64;
    CHECK(threads > 0 && threads <= 12 && loops > 0);
    if (argc > 4 && !strcmp(argv[4], "filtered")) {
        struct filter_instruction instruction = {.code = 6, .value = 0x7fff0000};
        struct filter_program filter = {.count = 1, .instructions = &instruction};
        CHECK(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0);
        CHECK(syscall(SYS_seccomp, 1, 0, &filter) == 0);
    }
    if (!strcmp(mode, "compat")) compatibility(argv);
    else CHECK(!strcmp(mode, "loops"));
    CHECK(pthread_barrier_init(&barrier, NULL, threads + 1) == 0); pthread_t ids[12];
    for (unsigned id = 0; id < threads; id++) CHECK(pthread_create(&ids[id], NULL, worker, (void *)(uintptr_t)id) == 0);
    wait_barrier(); wait_barrier();
    uint64_t longest = 0;
    for (unsigned id = 0; id < threads; id++) {
        CHECK(pthread_join(ids[id], NULL) == 0); if (durations[id] > longest) longest = durations[id];
    }
    CHECK(pthread_barrier_destroy(&barrier) == 0);
    printf("{\"mode\":\"%s\",\"threads\":%u,\"loops\":%u,\"elapsed_ns\":%llu}\n", mode, threads, loops, (unsigned long long)longest);
    return 0;
}
