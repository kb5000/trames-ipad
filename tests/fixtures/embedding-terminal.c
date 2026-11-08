// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum { THREADS = 4, STREAM_BYTES = 8 * 1024 * 1024 };
static unsigned rounds;
static pid_t group, session;
static atomic_int failure;
static atomic_uint_fast64_t received;
static uint64_t began;
static uint64_t now_ns(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t)) _exit(80);
    return (uint64_t)t.tv_sec * 1000000000 + (uint64_t)t.tv_nsec;
}
static void setup(void) {
    if (!isatty(0) || !isatty(1)) _exit(81);
    if (ioctl(0, TIOCSCTTY, 0)) _exit(82);
    struct termios attr;
    if (tcgetattr(0, &attr)) _exit(83);
    attr.c_lflag = (attr.c_lflag | ICANON) & ~(ECHO | ECHONL);
    attr.c_oflag |= OPOST | ONLCR;
    if (tcsetattr(0, TCSANOW, &attr)) _exit(84);
    struct winsize size = { .ws_row = 30, .ws_col = 100 };
    if (ioctl(0, TIOCSWINSZ, &size)) _exit(85);
    group = getpgrp();
    session = getsid(0);
    if (tcgetpgrp(0) != group || tcgetsid(0) != session) _exit(86);
}
static void *queries(void *arg) {
    (void)arg;
    for (unsigned i = 0; i < rounds; ++i) {
        struct termios attr;
        struct winsize size;
        if (tcgetattr(0, &attr) || tcgetpgrp(0) != group || tcgetsid(0) != session ||
            ioctl(0, TIOCGWINSZ, &size) || size.ws_row != 30 || size.ws_col != 100) {
            atomic_store(&failure, 1);
            break;
        }
    }
    return NULL;
}
static void *reader(void *arg) {
    (void)arg;
    char data[2];
    size_t at = 0;
    while (at != sizeof(data)) {
        ssize_t n = read(0, data + at, sizeof(data) - at);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { atomic_store(&failure, 2); return NULL; }
        at += (size_t)n;
    }
    if (memcmp(data, "x\n", 2)) atomic_store(&failure, 3);
    atomic_store(&received, now_ns() - began);
    return NULL;
}
static void *writer(void *arg) {
    (void)arg;
    char bytes[65536];
    memset(bytes, 'Z', sizeof(bytes));
    size_t total = 0;
    while (total != STREAM_BYTES) {
        size_t left = STREAM_BYTES - total;
        ssize_t n = write(1, bytes, left < sizeof(bytes) ? left : sizeof(bytes));
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { atomic_store(&failure, 4); break; }
        total += (size_t)n;
    }
    return NULL;
}
int main(int argc, char **argv) {
    setup();
    const char *mode = argc > 1 ? argv[1] : "queries";
    rounds = argc > 2 ? (unsigned)strtoul(argv[2], NULL, 10) : 1000;
    if (!strcmp(mode, "queries")) {
        pthread_t threads[THREADS];
        began = now_ns();
        for (unsigned i = 0; i < THREADS; ++i)
            if (pthread_create(&threads[i], NULL, queries, NULL)) _exit(87);
        for (unsigned i = 0; i < THREADS; ++i) pthread_join(threads[i], NULL);
        uint64_t elapsed = now_ns() - began;
        printf("{\"mode\":\"queries\",\"threads\":%d,\"rounds\":%u,\"elapsed_ns\":%llu,\"failure\":%d}\n",
               THREADS, rounds, (unsigned long long)elapsed, atomic_load(&failure));
    } else if (!strcmp(mode, "duplex")) {
        fputs("READY\n", stdout);
        fflush(stdout);
        began = now_ns();
        pthread_t in, out;
        if (pthread_create(&in, NULL, reader, NULL) || pthread_create(&out, NULL, writer, NULL)) _exit(88);
        const struct timespec delay = { .tv_nsec = 1000000 };
        while (!atomic_load(&received) && now_ns() - began < 2000000000) nanosleep(&delay, NULL);
        uint64_t latency = atomic_load(&received);
        if (!latency || latency > 2000000000) _exit(70);
        pthread_join(in, NULL);
        pthread_join(out, NULL);
        printf("\n{\"mode\":\"duplex\",\"bytes\":%d,\"input_ns\":%llu,\"failure\":%d}\n",
               STREAM_BYTES, (unsigned long long)latency, atomic_load(&failure));
    } else return 89;
    return atomic_load(&failure) != 0;
}
