/* SPDX-License-Identifier: GPL-3.0-or-later */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

/* Stable Linux UAPI layout, independent of the host libc's kernel headers. */
typedef unsigned long aio_context_t;
struct iocb {
    uint64_t aio_data;
    uint32_t aio_key, aio_rw_flags;
    uint16_t aio_lio_opcode;
    int16_t aio_reqprio;
    uint32_t aio_fildes;
    uint64_t aio_buf, aio_nbytes;
    int64_t aio_offset;
    uint64_t aio_reserved2;
    uint32_t aio_flags, aio_resfd;
};
struct io_event { uint64_t data, obj; int64_t res, res2; };
enum { IOCB_CMD_PREAD = 0, IOCB_CMD_PWRITE = 1, IOCB_CMD_FDSYNC = 3, IOCB_CMD_PWRITEV = 8, IOCB_FLAG_RESFD = 1 };
_Static_assert(sizeof(struct iocb) == 64 && sizeof(struct io_event) == 32, "Linux AIO ABI");

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "async-io:%d: %s: %s\n", __LINE__, #x, strerror(errno)); exit(1); } } while (0)
static volatile sig_atomic_t caught, seen_fd, seen_code;
static volatile long seen_band;
static void handler(int sig, siginfo_t *info, void *context) {
    (void)context;
    caught = sig;
    seen_fd = info->si_fd;
    seen_code = info->si_code;
    seen_band = info->si_band;
}
static void signals(void) {
    struct sigaction action = {.sa_sigaction = handler, .sa_flags = SA_SIGINFO};
    sigemptyset(&action.sa_mask);
    CHECK(sigaction(SIGIO, &action, NULL) == 0);
    CHECK(sigaction(SIGRTMIN + 1, &action, NULL) == 0);
    int pair[2]; CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    CHECK(fcntl(pair[0], F_SETOWN, getpid()) == 0);
    CHECK(fcntl(pair[0], F_GETOWN) == getpid());
    int enabled = 1; CHECK(ioctl(pair[0], FIOASYNC, &enabled) == 0);
    CHECK(fcntl(pair[0], F_GETFL) & O_ASYNC);
    CHECK(write(pair[1], "a", 1) == 1);
    char byte; CHECK(read(pair[0], &byte, 1) == 1 && byte == 'a');
    CHECK(caught == SIGIO);
    CHECK(fcntl(pair[0], F_SETSIG, SIGRTMIN + 1) == 0);
    CHECK(fcntl(pair[0], F_GETSIG) == SIGRTMIN + 1);
    caught = 0;
    CHECK(write(pair[1], "b", 1) == 1);
    CHECK(read(pair[0], &byte, 1) == 1 && byte == 'b');
    CHECK(caught == SIGRTMIN + 1 && seen_fd == pair[0] && (seen_band & POLLIN) && seen_code == POLL_IN);
    struct f_owner_ex owner = {.type = F_OWNER_TID, .pid = (pid_t)syscall(SYS_gettid)};
    CHECK(fcntl(pair[0], F_SETOWN_EX, &owner) == 0);
    memset(&owner, 0, sizeof(owner)); CHECK(fcntl(pair[0], F_GETOWN_EX, &owner) == 0);
    CHECK(owner.type == F_OWNER_TID && owner.pid == syscall(SYS_gettid));
    int alias = dup(pair[0]); CHECK(alias >= 0);
    CHECK(fcntl(alias, F_SETFL, fcntl(alias, F_GETFL) & ~O_ASYNC) == 0);
    caught = 0; CHECK(write(pair[1], "c", 1) == 1); CHECK(read(pair[0], &byte, 1) == 1); CHECK(caught == 0);
    close(alias); close(pair[0]); close(pair[1]);
    puts("PASS SIGIO FIOASYNC F_SETOWN F_SETSIG aliases");
}
static void aio(void) {
    aio_context_t context = 0;
    CHECK(syscall(SYS_io_setup, 8, &context) == 0 && context);
    struct aio_ring {
        uint32_t id, nr, head, tail, magic, compat, incompat, header_length;
    } *ring = (struct aio_ring *)(uintptr_t)context;
    CHECK(ring->magic == 0xa10a10a1 && ring->incompat == 0);
    CHECK(ring->nr > 8 && ring->header_length == sizeof(*ring));
    char path[] = "/tmp/trames-aio-XXXXXX"; int fd = mkstemp(path); CHECK(fd >= 0);
    int readonly = open(path, O_RDONLY); CHECK(readonly >= 0); unlink(path);
    CHECK(write(fd, "payload", 7) == 7);
    int event = eventfd(0, EFD_NONBLOCK); CHECK(event >= 0);
    char buffer[8] = {0};
    struct iocb cb = {.aio_data = 123, .aio_lio_opcode = IOCB_CMD_PREAD, .aio_fildes = fd, .aio_buf = (uintptr_t)buffer, .aio_nbytes = 7, .aio_flags = IOCB_FLAG_RESFD, .aio_resfd = event};
    struct iocb *list[] = {&cb};
    CHECK(syscall(SYS_io_submit, context, 1, list) == 1);
    struct io_event complete;
    struct timespec timeout = {.tv_sec = 5};
    CHECK(syscall(SYS_io_getevents, context, 1, 1, &complete, &timeout) == 1);
    CHECK(complete.data == 123 && complete.obj == (uintptr_t)&cb && complete.res == 7 && complete.res2 == 0);
    CHECK(strcmp(buffer, "payload") == 0);
    uint64_t count; CHECK(read(event, &count, 8) == 8 && count == 1);
    /* Consume a second completion in userspace. The following syscall must
       observe the advanced head and must not return the same record. */
    cb.aio_data = 124;
    CHECK(syscall(SYS_io_submit, context, 1, list) == 1);
    struct pollfd ready = {.fd = event, .events = POLLIN};
    CHECK(poll(&ready, 1, 5000) == 1 && read(event, &count, 8) == 8 && count == 1);
    uint32_t head = ring->head;
    CHECK(head < ring->nr && head != __atomic_load_n(&ring->tail, __ATOMIC_ACQUIRE));
    complete = ((struct io_event *)(ring + 1))[head];
    CHECK(complete.data == 124 && complete.obj == (uintptr_t)&cb && complete.res == 7);
    __atomic_store_n(&ring->head, (head + 1) % ring->nr, __ATOMIC_RELEASE);
    struct timespec immediate = {0};
    CHECK(syscall(SYS_io_getevents, context, 0, 1, &complete, &immediate) == 0);
    struct iovec parts[] = {{"abc", 3}, {"xyz", 3}};
    cb = (struct iocb){.aio_lio_opcode = IOCB_CMD_PWRITEV, .aio_fildes = fd, .aio_buf = (uintptr_t)parts, .aio_nbytes = 2, .aio_offset = 7};
    CHECK(syscall(SYS_io_submit, context, 1, list) == 1);
    CHECK(syscall(SYS_io_getevents, context, 1, 1, &complete, &timeout) == 1 && complete.res == 6);
    char written[7] = {0}; CHECK(pread(fd, written, 6, 7) == 6 && strcmp(written, "abcxyz") == 0);
    cb = (struct iocb){.aio_lio_opcode = IOCB_CMD_FDSYNC, .aio_fildes = fd};
    CHECK(syscall(SYS_io_submit, context, 1, list) == 1);
    CHECK(syscall(SYS_io_getevents, context, 1, 1, &complete, &timeout) == 1 && complete.res == 0);
    struct iocb bad = {.aio_lio_opcode = 0xffff}; struct iocb *partial[] = {&cb, &bad};
    CHECK(syscall(SYS_io_submit, context, 2, partial) == 1);
    CHECK(syscall(SYS_io_getevents, context, 1, 1, &complete, &timeout) == 1 && complete.res == 0);
    timeout = (struct timespec){0}; CHECK(syscall(SYS_io_getevents, context, 0, 1, &complete, &timeout) == 0);
    CHECK(syscall(SYS_io_cancel, context, &cb, &complete) == -1 && errno == EINVAL);
    cb = (struct iocb){.aio_lio_opcode = IOCB_CMD_PWRITE, .aio_fildes = readonly, .aio_buf = (uintptr_t)buffer, .aio_nbytes = 7};
    CHECK(syscall(SYS_io_submit, context, 1, list) == -1 && errno == EBADF);
    CHECK(syscall(SYS_io_destroy, context) == 0);
    CHECK(syscall(SYS_io_destroy, context) == -1 && errno == EINVAL);
    close(event); close(readonly); close(fd); puts("PASS AIO scalar vector sync partial CQ eventfd lifetime");
}
static void exclusive(void) {
    int ep = epoll_create1(0), nested = epoll_create1(0), event = eventfd(0, 0); CHECK(ep >= 0 && nested >= 0 && event >= 0);
    struct epoll_event interest = {.events = EPOLLEXCLUSIVE | EPOLLIN, .data.u64 = 42}, ready;
    CHECK(epoll_ctl(ep, EPOLL_CTL_ADD, nested, &interest) == -1 && errno == EINVAL);
    CHECK(epoll_ctl(ep, EPOLL_CTL_ADD, event, &interest) == 0);
    interest.events = EPOLLIN; CHECK(epoll_ctl(ep, EPOLL_CTL_MOD, event, &interest) == -1 && errno == EINVAL);
    uint64_t one = 1; CHECK(write(event, &one, 8) == 8);
    CHECK(epoll_wait(ep, &ready, 1, 1000) == 1 && ready.data.u64 == 42 && (ready.events & EPOLLIN));
    CHECK(epoll_ctl(ep, EPOLL_CTL_DEL, event, NULL) == 0);
    close(event); close(nested); close(ep); puts("PASS EPOLLEXCLUSIVE validation readiness deletion");
}
int main(int argc, char **argv) {
    if (argc == 1 || !strcmp(argv[1], "signals")) signals();
    if (argc == 1 || !strcmp(argv[1], "aio")) aio();
    if (argc == 1 || !strcmp(argv[1], "epoll")) exclusive();
    return 0;
}
