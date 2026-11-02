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
#include <fcntl.h>
#include <sched.h>
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

static int tcp_socket(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); assert(fd >= 0);
    assert(!fcntl(fd, F_SETFL, O_NONBLOCK));
    return fd;
}
static void tcp_read(int fd, unsigned char *byte, int eof) {
    double deadline = now() + 5;
    for (;;) {
        ssize_t n = read(fd, byte, 1);
        if (n >= 0) { assert(n == (eof ? 0 : 1)); return; }
        assert(errno == EAGAIN || errno == EINTR);
        assert(now() < deadline); sched_yield();
    }
}
int main(int argc, char **argv) {
    assert(argc == 3);
    const char *kind = argv[1];
    unsigned operations = (unsigned)strtoul(argv[2], NULL, 10);
    assert(operations && operations <= 1000000);
    uint64_t checksum = 0;
    int listener = -1;
    struct sockaddr_in address = {0};
    if (!strcmp(kind, "tcp-session")) {
        listener = tcp_socket(); address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        assert(!bind(listener, (struct sockaddr *)&address, sizeof(address)));
        socklen_t size = sizeof(address);
        assert(!getsockname(listener, (struct sockaddr *)&address, &size));
        assert(!listen(listener, 4));
    }
    double began = now();
    for (unsigned i = 0; i < operations; i++) {
        if (!strcmp(kind, "tcp-session")) {
            int client = tcp_socket();
            int result = connect(client, (struct sockaddr *)&address, sizeof(address));
            assert(!result || errno == EINPROGRESS);
            int peer; double deadline = now() + 5;
            while ((peer = accept(listener, NULL, NULL)) < 0) {
                assert(errno == EAGAIN || errno == EINTR); assert(now() < deadline); sched_yield();
            }
            assert(!fcntl(peer, F_SETFL, O_NONBLOCK));
            int error = 0; socklen_t size = sizeof(error);
            assert(!getsockopt(client, SOL_SOCKET, SO_ERROR, &error, &size) && !error);
            unsigned char sent = (unsigned char)i, received = 0;
            assert(write(client, &sent, 1) == 1); tcp_read(peer, &received, 0); assert(received == sent);
            assert(!close(client)); tcp_read(peer, &received, 1); assert(!close(peer));
        } else if (!strcmp(kind, "tcp") || !strcmp(kind, "tcp-state")) {
            int fd = socket(AF_INET, SOCK_STREAM, 0); assert(fd >= 0);
            int enabled = 1, value = 0;
            if (!strcmp(kind, "tcp-state")) {
                socklen_t size = sizeof(value);
                assert(!setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled)));
                assert(!getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &value, &size) && value);
                assert(!getsockopt(fd, SOL_SOCKET, SO_ERROR, &value, &size) && !value);
                int bytes = 65536;
                assert(!setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes)));
                assert(!getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, &size) && value >= bytes);
            }
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
    if (listener >= 0) assert(!close(listener));
    assert(checksum == (uint64_t)operations * (operations + 1) / 2);
    printf("{\"kind\":\"%s\",\"operations\":%u,\"seconds\":%.9f,\"checksum\":%llu}\n",
           kind, operations, elapsed, (unsigned long long)checksum);
    return 0;
}
