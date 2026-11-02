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
#include <poll.h>
#ifdef __linux__
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/timerfd.h>
#elif defined(__APPLE__)
#include <sys/event.h>
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
static int tcp_connect(int fd, const struct sockaddr_in *address) {
    int result = connect(fd, (const struct sockaddr *)address, sizeof(*address));
    int error = errno;
    if (result && error != EINPROGRESS)
        fprintf(stderr, "TCP connect failed: errno=%d (%s)\n", error, strerror(error));
    assert(!result || error == EINPROGRESS);
    return result;
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
static int queue_create(void) {
#ifdef __linux__
    return epoll_create1(0);
#else
    return kqueue();
#endif
}
static void interest(int queue, int operation, int fd, uint64_t data, int oneshot) {
#ifdef __linux__
    struct epoll_event event = {.events = EPOLLIN | (oneshot ? EPOLLONESHOT : 0), .data.u64 = data};
    assert(!epoll_ctl(queue, operation, fd, operation == 2 ? NULL : &event));
#else
    struct kevent event;
    unsigned flags = operation == 2 ? EV_DELETE : EV_ADD | EV_ENABLE | (oneshot ? EV_DISPATCH : 0);
    EV_SET(&event, fd, EVFILT_READ, flags, 0, 0, (void *)(uintptr_t)data);
    assert(!kevent(queue, &event, 1, NULL, 0, NULL));
#endif
}
static uint64_t queue_wait(int queue) {
#ifdef __linux__
    struct epoll_event event;
    assert(epoll_wait(queue, &event, 1, 0) == 1 && event.events & EPOLLIN);
    return event.data.u64;
#else
    struct kevent event;
    struct timespec zero = {0};
    assert(kevent(queue, NULL, 0, &event, 1, &zero) == 1 && event.filter == EVFILT_READ);
    return (uint64_t)(uintptr_t)event.udata;
#endif
}
static uint64_t tcp_queue_wait(int queue) {
#ifdef __linux__
    struct epoll_event event;
    assert(epoll_wait(queue, &event, 1, 5000) == 1 && event.events & EPOLLIN);
    return event.data.u64;
#else
    struct kevent event;
    struct timespec timeout = {5, 0};
    assert(kevent(queue, NULL, 0, &event, 1, &timeout) == 1 && event.filter == EVFILT_READ);
    return (uint64_t)(uintptr_t)event.udata;
#endif
}
int main(int argc, char **argv) {
    assert(argc == 3);
    const char *kind = argv[1];
    unsigned operations = (unsigned)strtoul(argv[2], NULL, 10);
    assert(operations && operations <= 1000000);
    uint64_t checksum = 0;
    int listener = -1;
    struct sockaddr_in address = {0};
    int tcp_blocking = !strcmp(kind, "tcp-blocking");
    int tcp_events = !strcmp(kind, "tcp-epoll") || !strcmp(kind, "tcp-poll");
    if (!strcmp(kind, "tcp-session") || tcp_blocking || tcp_events) {
        listener = tcp_socket(); address.sin_family = AF_INET;
        if (tcp_blocking) assert(!fcntl(listener, F_SETFL, 0));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        assert(!bind(listener, (struct sockaddr *)&address, sizeof(address)));
        socklen_t size = sizeof(address);
        assert(!getsockname(listener, (struct sockaddr *)&address, &size));
        assert(!listen(listener, 4));
    }
    int queue = -1, pair[2] = {-1,-1}, monitor = -1, local[2] = {-1,-1};
    char watched[] = "/tmp/trames-watch-XXXXXX";
    int signal_rights = 0;
#ifdef __linux__
    signal_rights = !strcmp(kind, "signal-rights");
    sigset_t signal_mask; assert(!sigemptyset(&signal_mask)); assert(!sigaddset(&signal_mask, SIGUSR1));
#endif
    if (!strcmp(kind, "unix-rights") || signal_rights) {
        assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, local));
#ifdef __linux__
        if (signal_rights) { pair[1] = signalfd(-1, &signal_mask, SFD_NONBLOCK); assert(pair[1] >= 0); }
        else
#endif
        assert(!pipe(pair));
    } else if (!strcmp(kind, "epoll")) {
        queue = queue_create(); assert(queue >= 0); assert(!pipe(pair));
    } else if (!strcmp(kind, "inotify-watch")) {
        assert(mkdtemp(watched));
#ifdef __linux__
        queue = inotify_init1(IN_NONBLOCK); assert(queue >= 0);
#else
        queue = kqueue(); assert(queue >= 0);
        monitor = open(watched, O_EVTONLY); assert(monitor >= 0);
#endif
    }
    if (tcp_events) {
        pair[0] = tcp_socket();
        if (!strcmp(kind, "tcp-epoll")) {
            queue = queue_create(); assert(queue >= 0);
            interest(queue, 1, pair[0], 0, 1); // register before connection
        }
        tcp_connect(pair[0], &address);
        double deadline = now() + 5;
        while ((pair[1] = accept(listener, NULL, NULL)) < 0) {
            assert(errno == EAGAIN || errno == EINTR); assert(now() < deadline); sched_yield();
        }
        int error = 0; socklen_t size = sizeof(error);
        assert(!getsockopt(pair[0], SOL_SOCKET, SO_ERROR, &error, &size) && !error);
    }
    double began = now();
    for (unsigned i = 0; i < operations; i++) {
        if (!strcmp(kind, "unix-rights") || signal_rights) {
            unsigned char sent = (unsigned char)i, received = 0;
            struct iovec vector = {.iov_base = &sent, .iov_len = 1};
            union { struct cmsghdr align; unsigned char bytes[CMSG_SPACE(sizeof(int))]; } control = {0};
            struct msghdr message = {.msg_iov = &vector, .msg_iovlen = 1,
                .msg_control = control.bytes, .msg_controllen = sizeof(control.bytes)};
            struct cmsghdr *header = CMSG_FIRSTHDR(&message);
            header->cmsg_len = CMSG_LEN(sizeof(int)); header->cmsg_level = SOL_SOCKET; header->cmsg_type = SCM_RIGHTS;
            memcpy(CMSG_DATA(header), &pair[1], sizeof(int));
            assert(sendmsg(local[0], &message, 0) == 1);
            memset(&control, 0, sizeof(control)); vector.iov_base = &received;
            assert(recvmsg(local[1], &message, 0) == 1 && received == sent && !message.msg_flags);
            header = CMSG_FIRSTHDR(&message);
            assert(header && header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS && header->cmsg_len == CMSG_LEN(sizeof(int)));
            int imported; memcpy(&imported, CMSG_DATA(header), sizeof(int));
#ifdef __linux__
            if (signal_rights) {
                assert(signalfd(imported, &signal_mask, 0) == imported);
                assert(fcntl(imported, F_GETFL) & O_NONBLOCK);
            } else
#endif
            {
                assert(write(imported, &sent, 1) == 1);
                assert(read(pair[0], &received, 1) == 1 && received == sent);
            }
            assert(!close(imported));
        } else if (tcp_events) {
            if (queue >= 0) interest(queue, 3, pair[0], (uint64_t)i + 1, 1);
            unsigned char sent = (unsigned char)i, received = 0;
            assert(write(pair[1], &sent, 1) == 1);
            if (queue >= 0) {
                assert(tcp_queue_wait(queue) == (uint64_t)i + 1);
            } else {
                struct pollfd event = {.fd = pair[0], .events = POLLIN};
                assert(poll(&event, 1, 5000) == 1 && event.revents & POLLIN);
            }
            assert(read(pair[0], &received, 1) == 1 && received == sent);
        } else if (!strcmp(kind, "epoll")) {
            interest(queue, 1, pair[0], 0, 0);
            interest(queue, 3, pair[0], (uint64_t)i + 1, 1);
            unsigned char sent = (unsigned char)i, received = 0;
            assert(write(pair[1], &sent, 1) == 1);
            assert(queue_wait(queue) == (uint64_t)i + 1);
            assert(read(pair[0], &received, 1) == 1 && received == sent);
            interest(queue, 2, pair[0], 0, 0);
        } else if (!strcmp(kind, "inotify-watch")) {
#ifdef __linux__
            int wd = inotify_add_watch(queue, watched, IN_CREATE); assert(wd >= 0);
            assert(!inotify_rm_watch(queue, wd));
            unsigned char bytes[64]; ssize_t n = read(queue, bytes, sizeof(bytes));
            assert(n >= (ssize_t)sizeof(struct inotify_event));
            const struct inotify_event *event = (const struct inotify_event *)bytes;
            assert(event->wd == wd && event->mask & IN_IGNORED);
#else
            struct kevent event; struct timespec zero = {0};
            EV_SET(&event, monitor, EVFILT_VNODE, EV_ADD | EV_CLEAR, NOTE_WRITE, 0, NULL);
            assert(!kevent(queue, &event, 1, NULL, 0, NULL));
            EV_SET(&event, monitor, EVFILT_VNODE, EV_DELETE, 0, 0, NULL);
            assert(!kevent(queue, &event, 1, NULL, 0, NULL));
            assert(!kevent(queue, NULL, 0, &event, 1, &zero));
#endif
        } else if (!strcmp(kind, "tcp-session") || tcp_blocking) {
            int client = tcp_socket();
            if (tcp_blocking) assert(!fcntl(client, F_SETFL, 0));
            int result = tcp_connect(client, &address);
            if (tcp_blocking) assert(!result);
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
        } else if (!strcmp(kind, "signalfd")) {
            int fd = signalfd(-1, &signal_mask, SFD_NONBLOCK | SFD_CLOEXEC); assert(fd >= 0);
            int alias = dup(fd); assert(alias >= 0); assert(!close(fd));
            sigset_t changed; assert(!sigemptyset(&changed)); assert(!sigaddset(&changed, SIGUSR2));
            assert(signalfd(alias, &changed, 0) == alias);
            assert(!fcntl(alias, F_GETFD)); assert(!close(alias));
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
    if (queue >= 0) assert(!close(queue));
    if (pair[0] >= 0) assert(!close(pair[0]));
    if (pair[1] >= 0) assert(!close(pair[1]));
    if (local[0] >= 0) {assert(!close(local[0])); assert(!close(local[1]));}
    if (monitor >= 0) assert(!close(monitor));
    if (!strcmp(kind, "inotify-watch")) assert(!rmdir(watched));
    assert(checksum == (uint64_t)operations * (operations + 1) / 2);
    printf("{\"kind\":\"%s\",\"operations\":%u,\"seconds\":%.9f,\"checksum\":%llu}\n",
           kind, operations, elapsed, (unsigned long long)checksum);
    return 0;
}
