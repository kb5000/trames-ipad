// SPDX-License-Identifier: GPL-3.0-or-later
// Fixed-work descriptor creation, I/O/control and final close.
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#endif

static double now(void) {
    struct timespec t;
    assert(!clock_gettime(CLOCK_MONOTONIC, &t));
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
    assert(argc == 3);
    const char *kind = argv[1];
    unsigned operations = (unsigned)strtoul(argv[2], NULL, 10);
    assert(operations && operations <= 1000000);
    uint64_t checksum = 0;
    double began = now();
    for (unsigned i = 0; i < operations; i++) {
        if (!strcmp(kind, "tcp")) {
            int fd = socket(AF_INET, SOCK_STREAM, 0); assert(fd >= 0);
            int enabled = 1, value = 0;
            assert(!setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled)));
            socklen_t size = sizeof(value);
            assert(!getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &value, &size));
            assert(value && size == sizeof(value));
            struct sockaddr_in address; size = sizeof(address);
            assert(!getsockname(fd, (struct sockaddr *)&address, &size));
            assert(address.sin_family == AF_INET && size == sizeof(address));
            assert(!close(fd));
        } else if (!strcmp(kind, "pipe")) {
            int p[2];
#ifdef __linux__
            assert(!pipe2(p, 0));
#else
            assert(!pipe(p));
#endif
            unsigned char sent = (unsigned char)i, received = 0;
            assert(write(p[1], &sent, 1) == 1);
            assert(read(p[0], &received, 1) == 1 && received == sent);
            assert(!close(p[1]));
            assert(read(p[0], &received, 1) == 0);
            assert(!close(p[0]));
#ifdef __linux__
        } else if (!strcmp(kind, "eventfd")) {
            int fd = eventfd(0, 0); assert(fd >= 0);
            uint64_t sent = (uint64_t)i + 1, received = 0;
            assert(write(fd, &sent, 8) == 8);
            assert(read(fd, &received, 8) == 8 && received == sent);
            assert(!close(fd));
        } else if (!strcmp(kind, "timerfd")) {
            int fd = timerfd_create(CLOCK_MONOTONIC, 0); assert(fd >= 0);
            struct itimerspec value = {0}, current;
            assert(!timerfd_settime(fd, 0, &value, NULL));
            assert(!timerfd_gettime(fd, &current));
            assert(!current.it_interval.tv_sec && !current.it_interval.tv_nsec);
            assert(!current.it_value.tv_sec && !current.it_value.tv_nsec);
            assert(!close(fd));
        } else if (!strcmp(kind, "inotify")) {
            int fd = inotify_init1(IN_NONBLOCK); assert(fd >= 0);
            unsigned char bytes[64];
            assert(read(fd, bytes, sizeof(bytes)) == -1 && errno == EAGAIN);
            assert(!close(fd));
#endif
        } else {
            fprintf(stderr, "unsupported descriptor kind: %s\n", kind);
            return 77;
        }
        checksum += (uint64_t)i + 1;
    }
    double elapsed = now() - began;
    assert(checksum == (uint64_t)operations * (operations + 1) / 2);
    printf("{\"kind\":\"%s\",\"operations\":%u,\"seconds\":%.9f,\"checksum\":%llu}\n",
           kind, operations, elapsed, (unsigned long long)checksum);
    return 0;
}
