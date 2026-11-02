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
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/ioctl.h>
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
    int unix_listener = -1;
    int unix_session = !strcmp(kind, "unix-session") || !strcmp(kind, "unix-abstract");
    int unix_abstract = !strcmp(kind, "unix-abstract");
    struct sockaddr_un unix_address = {0};
    socklen_t unix_size = 0;
    if (unix_session) {
        unix_address.sun_family = AF_UNIX;
        if (unix_abstract) {
#ifdef __linux__
            snprintf(unix_address.sun_path + 1, sizeof(unix_address.sun_path) - 1, "trames-bench-%ld", (long)getpid());
            unix_size = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(unix_address.sun_path + 1));
#else
            fprintf(stderr, "abstract Unix sockets require Linux\n"); return 77;
#endif
        } else {
            snprintf(unix_address.sun_path, sizeof(unix_address.sun_path), "/tmp/trames-bench-%ld", (long)getpid());
            unix_size = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(unix_address.sun_path) + 1);
            assert(!unlink(unix_address.sun_path) || errno == ENOENT);
        }
#ifdef __APPLE__
        unix_address.sun_len = (uint8_t)unix_size;
#endif
        unix_listener = socket(AF_UNIX, SOCK_STREAM, 0); assert(unix_listener >= 0);
        assert(!bind(unix_listener, (struct sockaddr *)&unix_address, unix_size));
        assert(!listen(unix_listener, 4));
    }
    int datagram = !strcmp(kind, "unix-datagram") || !strcmp(kind, "unix-sendto") || !strcmp(kind, "unix-datagram-rights") || !strcmp(kind, "unix-datagram-owner");
    int addressed = !strcmp(kind, "unix-sendto");
    int datagram_owner = !strcmp(kind, "unix-datagram-owner");
    int datagram_rights = !strcmp(kind, "unix-datagram-rights") || datagram_owner;
    int right_source = -1;
    if (datagram_rights && !datagram_owner) {
        int ends[2]; assert(!pipe(ends)); assert(!close(ends[1])); right_source = ends[0];
    }
    int packets[2] = {-1, -1};
    struct sockaddr_un packet_address[2] = {{0}, {0}};
    socklen_t packet_size[2] = {0, 0};
    if (datagram) {
        for (unsigned j = 0; j < 2; j++) {
            packet_address[j].sun_family = AF_UNIX;
            snprintf(packet_address[j].sun_path, sizeof(packet_address[j].sun_path), "/tmp/trames-datagram-%ld-%u", (long)getpid(), j);
            packet_size[j] = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(packet_address[j].sun_path) + 1);
#ifdef __APPLE__
            packet_address[j].sun_len = (uint8_t)packet_size[j];
#endif
            assert(!unlink(packet_address[j].sun_path) || errno == ENOENT);
            packets[j] = socket(AF_UNIX, SOCK_DGRAM, 0); assert(packets[j] >= 0);
            assert(!bind(packets[j], (struct sockaddr *)&packet_address[j], packet_size[j]));
        }
        if (!addressed) assert(!connect(packets[0], (struct sockaddr *)&packet_address[1], packet_size[1]));
    }
    int unix_batch = !strcmp(kind, "unix-batch");
    int unix_packet = !strcmp(kind, "unix-packet") || !strcmp(kind, "unix-packet-vector");
    int packet_vector = !strcmp(kind, "unix-packet-vector");
    int packet_pair[2] = {-1, -1};
    if (unix_batch || unix_packet) assert(!socketpair(AF_UNIX, SOCK_DGRAM, 0, packet_pair));
    int positioned_file = -1;
    char positioned_path[96] = {0};
    if (!strcmp(kind, "file-positioned")) {
        snprintf(positioned_path, sizeof(positioned_path), "/tmp/trames-file-%ld", (long)getpid());
        positioned_file = open(positioned_path, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600); assert(positioned_file >= 0);
    }
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
    int pty_rights = 0, pty_lifecycle = 0, pty_peer = 0, pty_create = 0;
#ifdef __linux__
    signal_rights = !strcmp(kind, "signal-rights");
    pty_rights = !strcmp(kind, "pty-rights");
    pty_lifecycle = !strcmp(kind, "pty");
    pty_peer = !strcmp(kind, "pty-peer");
    pty_create = !strcmp(kind, "pty-create");
    const char *ptmx = getenv("TRAMES_BENCH_PTMX");
    if (!ptmx) ptmx = "/dev/ptmx";
    sigset_t signal_mask; assert(!sigemptyset(&signal_mask)); assert(!sigaddset(&signal_mask, SIGUSR1));
    if (pty_rights || pty_lifecycle || pty_peer) {
        pair[0] = open(ptmx, O_RDWR | O_NOCTTY); assert(pair[0] >= 0);
        int unlocked = 0; assert(!ioctl(pair[0], TIOCSPTLCK, &unlocked));
        if (!pty_peer) { pair[1] = ioctl(pair[0], TIOCGPTPEER, O_RDWR | O_NOCTTY); assert(pair[1] >= 0); }
    }
#endif
    if (!strcmp(kind, "unix-rights") || signal_rights || pty_rights) {
        assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, local));
#ifdef __linux__
        if (signal_rights) { pair[1] = signalfd(-1, &signal_mask, SFD_NONBLOCK); assert(pair[1] >= 0); }
        else
#endif
        if (!pty_rights) assert(!pipe(pair));
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
        if (positioned_file >= 0) {
            unsigned char sent[64], received[64]; memset(sent, (unsigned char)i, sizeof(sent));
            assert(pwrite(positioned_file, sent, sizeof(sent), 0) == (ssize_t)sizeof(sent));
            assert(pread(positioned_file, received, sizeof(received), 0) == (ssize_t)sizeof(received));
            assert(!memcmp(sent, received, sizeof(sent)));
        } else if (!strncmp(kind, "device-", 7)) {
            char path[64]; snprintf(path, sizeof(path), "/dev/%s", kind + 7);
            int fd = open(path, O_RDWR | O_CLOEXEC); assert(fd >= 0);
            int null = !strcmp(kind, "device-null"), full = !strcmp(kind, "device-full");
            unsigned char bytes[64]; memset(bytes, (unsigned char)i, sizeof(bytes));
            ssize_t written = write(fd, bytes, sizeof(bytes));
            assert(full ? written == -1 && errno == ENOSPC : written == (ssize_t)sizeof(bytes));
            assert(read(fd, bytes, sizeof(bytes)) == (null ? 0 : (ssize_t)sizeof(bytes)));
            if (!strcmp(kind, "device-zero") || full) for (unsigned j = 0; j < sizeof(bytes); j++) assert(bytes[j] == 0);
            written = pwrite(fd, bytes, sizeof(bytes), 33);
            assert(full ? written == -1 && errno == ENOSPC : written == (ssize_t)sizeof(bytes));
            assert(pread(fd, bytes, sizeof(bytes), 47) == (null ? 0 : (ssize_t)sizeof(bytes)));
            struct iovec vectors[2] = {{.iov_base = &bytes[0], .iov_len = 1}, {.iov_base = &bytes[1], .iov_len = 1}};
            written = writev(fd, vectors, 2);
            assert(full ? written == -1 && errno == ENOSPC : written == 2);
            assert(readv(fd, vectors, 2) == (null ? 0 : 2));
#ifdef __APPLE__
            assert(lseek(fd, 27, SEEK_END) == 27); // Native BSD device seek semantics.
#else
            assert(lseek(fd, 27, SEEK_END) == 0);
#endif
            struct stat stat; assert(!fstat(fd, &stat) && S_ISCHR(stat.st_mode));
            assert(!close(fd));
        } else if (unix_batch) {
            unsigned char sent[8], received[8] = {0};
            struct iovec vectors[8];
#ifdef __linux__
            struct mmsghdr messages[8] = {0};
#else
            struct msghdr messages[8] = {0};
#endif
            for (unsigned j = 0; j < 8; j++) {
                sent[j] = (unsigned char)(i + j);
                vectors[j] = (struct iovec){.iov_base = &sent[j], .iov_len = 1};
#ifdef __linux__
                messages[j].msg_hdr = (struct msghdr){.msg_iov = &vectors[j], .msg_iovlen = 1};
#else
                messages[j] = (struct msghdr){.msg_iov = &vectors[j], .msg_iovlen = 1};
#endif
            }
#ifdef __linux__
            assert(sendmmsg(packet_pair[0], messages, 8, 0) == 8);
            for (unsigned j = 0; j < 8; j++) assert(messages[j].msg_len == 1);
#else
            // Contextual native reference: eight POSIX calls, not a batch API.
            for (unsigned j = 0; j < 8; j++) assert(sendmsg(packet_pair[0], &messages[j], 0) == 1);
#endif
            for (unsigned j = 0; j < 8; j++) assert(recv(packet_pair[1], &received[j], 1, 0) == 1 && received[j] == sent[j]);
        } else if (unix_packet) {
            unsigned char sent[2] = {(unsigned char)i, (unsigned char)(i + 1)}, received[2] = {0};
            if (packet_vector) {
                struct iovec vectors[2] = {{.iov_base = &sent[0], .iov_len = 1}, {.iov_base = &sent[1], .iov_len = 1}};
                assert(writev(packet_pair[0], vectors, 2) == 2);
                vectors[0].iov_base = &received[0]; vectors[1].iov_base = &received[1];
                assert(readv(packet_pair[1], vectors, 2) == 2);
            } else {
                assert(write(packet_pair[0], sent, 2) == 2);
                assert(read(packet_pair[1], received, 2) == 2);
            }
            assert(!memcmp(sent, received, 2));
        } else if (datagram) {
            unsigned char sent = (unsigned char)i, received = 0;
            if (addressed) {
                struct sockaddr_un source = {0}; socklen_t size = sizeof(source);
                assert(sendto(packets[0], &sent, 1, 0, (struct sockaddr *)&packet_address[1], packet_size[1]) == 1);
                assert(recvfrom(packets[1], &received, 1, 0, (struct sockaddr *)&source, &size) == 1);
                assert(size == packet_size[0] && !memcmp(source.sun_path, packet_address[0].sun_path, strlen(packet_address[0].sun_path) + 1));
            } else if (datagram_rights) {
                struct iovec vector = {.iov_base = &sent, .iov_len = 1};
                union { struct cmsghdr align; unsigned char bytes[CMSG_SPACE(sizeof(int))]; } control = {0};
                struct msghdr message = {.msg_iov = &vector, .msg_iovlen = 1, .msg_control = control.bytes, .msg_controllen = sizeof(control.bytes)};
                struct cmsghdr *header = CMSG_FIRSTHDR(&message);
                header->cmsg_level = SOL_SOCKET; header->cmsg_type = SCM_RIGHTS; header->cmsg_len = CMSG_LEN(sizeof(int));
                int original = datagram_owner ? open("/dev/null", O_RDONLY) : right_source; assert(original >= 0);
                memcpy(CMSG_DATA(header), &original, sizeof(original));
                assert(sendmsg(packets[0], &message, 0) == 1); if (datagram_owner) assert(!close(original));
                vector.iov_base = &received;
                memset(control.bytes, 0, sizeof(control.bytes));
                assert(recvmsg(packets[1], &message, 0) == 1);
                header = CMSG_FIRSTHDR(&message);
                assert(header && header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS && header->cmsg_len == CMSG_LEN(sizeof(int)));
                int alias = -1; memcpy(&alias, CMSG_DATA(header), sizeof(alias));
                assert(alias >= 0 && read(alias, &sent, 1) == 0 && !close(alias));
            } else {
                assert(write(packets[0], &sent, 1) == 1);
                assert(read(packets[1], &received, 1) == 1);
            }
            assert(received == sent);
        } else if (unix_session) {
            int client = socket(AF_UNIX, SOCK_STREAM, 0); assert(client >= 0);
            assert(!connect(client, (struct sockaddr *)&unix_address, unix_size));
            int peer = accept(unix_listener, NULL, NULL); assert(peer >= 0);
            int alias = dup(peer); assert(alias >= 0); assert(!close(peer));
            unsigned char sent = (unsigned char)i, received = 0;
            assert(write(client, &sent, 1) == 1);
            assert(read(alias, &received, 1) == 1 && received == sent);
            assert(!close(client)); assert(read(alias, &received, 1) == 0); assert(!close(alias));
        } else if (!strcmp(kind, "unix-rights") || signal_rights || pty_rights) {
            unsigned char sent = pty_rights ? (unsigned char)('a' + i % 26) : (unsigned char)i, received = 0;
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
        } else if (pty_peer || pty_create) {
#ifdef __linux__
            int master = pair[0];
            if (pty_create) {
                master = open(ptmx, O_RDWR | O_NOCTTY); assert(master >= 0);
                int unlocked = 0; assert(!ioctl(master, TIOCSPTLCK, &unlocked));
            }
            int slave = ioctl(master, TIOCGPTPEER, O_RDWR | O_NOCTTY | O_CLOEXEC); assert(slave >= 0);
            assert(fcntl(slave, F_GETFD) & FD_CLOEXEC);
            unsigned char sent = (unsigned char)('a' + i % 26), received = 0;
            assert(write(slave, &sent, 1) == 1);
            assert(read(master, &received, 1) == 1 && received == sent);
            assert(!close(slave));
            if (pty_create) assert(!close(master));
#endif
        } else if (pty_lifecycle) {
#ifdef __linux__
            int alias = dup(pair[1]); assert(alias >= 0);
            assert((fcntl(alias, F_GETFL) & O_ACCMODE) == O_RDWR);
            int number = -1, locked = -1, packet = -1;
            assert(!ioctl(pair[0], TIOCGPTN, &number) && number >= 0);
            assert(!ioctl(pair[0], TIOCGPTLCK, &locked) && !locked);
            assert(!ioctl(pair[0], TIOCGPKT, &packet) && !packet);
            assert(!close(alias));
#endif
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
        } else if (!strcmp(kind, "unix-create") || !strcmp(kind, "unix-pair")) {
            int endpoints[2] = {-1, -1};
            if (!strcmp(kind, "unix-pair")) {
                assert(!socketpair(AF_UNIX, SOCK_STREAM, 0, endpoints));
            } else {
                endpoints[0] = socket(AF_UNIX, SOCK_STREAM, 0); assert(endpoints[0] >= 0);
            }
            int alias = dup(endpoints[0]); assert(alias >= 0);
            assert(!fcntl(alias, F_SETFL, O_NONBLOCK));
            assert(fcntl(endpoints[0], F_GETFL) & O_NONBLOCK);
            int bytes = 8192, value = 0; socklen_t size = sizeof(value);
            assert(!setsockopt(alias, SOL_SOCKET, SO_RCVBUF, &bytes, sizeof(bytes)));
            assert(!getsockopt(alias, SOL_SOCKET, SO_TYPE, &value, &size) && value == SOCK_STREAM);
            if (endpoints[1] >= 0) {
                unsigned char sent = (unsigned char)i, received = 0;
                assert(write(alias, &sent, 1) == 1);
                assert(read(endpoints[1], &received, 1) == 1 && received == sent);
                assert(!shutdown(alias, SHUT_WR));
                assert(read(endpoints[1], &received, 1) == 0);
                assert(!close(endpoints[1]));
            }
            assert(!close(alias)); assert(!close(endpoints[0]));
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
    if (positioned_file >= 0) { assert(!close(positioned_file)); assert(!unlink(positioned_path)); }
    if (packet_pair[0] >= 0) { assert(!close(packet_pair[0])); assert(!close(packet_pair[1])); }
    if (right_source >= 0) assert(!close(right_source));
    if (datagram) {
        for (unsigned j = 0; j < 2; j++) { assert(!close(packets[j])); assert(!unlink(packet_address[j].sun_path)); }
    }
    if (unix_listener >= 0) {
        assert(!close(unix_listener));
        if (!unix_abstract) assert(!unlink(unix_address.sun_path));
    }
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
