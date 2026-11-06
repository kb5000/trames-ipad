// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno); exit(1); } } while (0)
struct instruction { uint16_t code; uint8_t yes, no; uint32_t value; };
struct program { uint16_t count; struct instruction *instructions; };
static pthread_barrier_t barrier;
static unsigned threads, loops;
static int utsfd, netfd, sentinel;
static const struct sockaddr_un address = {.sun_family = AF_UNIX, .sun_path = "\0trames-task-ns"};
static const socklen_t address_len = offsetof(struct sockaddr_un, sun_path) + 15;
static char root[128], original_name[65];
static uint64_t elapsed[12];
static uint64_t now(void) {
    struct timespec t; CHECK(clock_gettime(CLOCK_MONOTONIC, &t) == 0);
    return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}
static void rendezvous(void) {
    int r = pthread_barrier_wait(&barrier); CHECK(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD);
}
static void *worker(void *arg) {
    unsigned id = (unsigned)(uintptr_t)arg;
    char path[160], name[40], cwd[180];
    snprintf(path, sizeof path, "%s/%u", root, id);
    snprintf(name, sizeof name, "task-%u", id);
    CHECK(mkdir(path, 0700) == 0);
    CHECK(unshare(CLONE_FILES | CLONE_FS) == 0);
    CHECK(close(sentinel) == 0);
    int local = eventfd(7, EFD_CLOEXEC); CHECK(local >= 0);
    mode_t mask = (mode_t)(id & 7); umask(mask);
    rendezvous(); uint64_t began = now();
    for (unsigned i = 0; i < loops; i++) {
        CHECK(unshare(CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWIPC | CLONE_NEWCGROUP) == 0);
        int sock = socket(AF_UNIX, SOCK_DGRAM, 0); CHECK(sock >= 0);
        CHECK(bind(sock, (const struct sockaddr *)&address, address_len) == 0);
        CHECK(close(sock) == 0);
        CHECK(sethostname(name, strlen(name)) == 0);
        struct utsname u; CHECK(uname(&u) == 0 && !strcmp(u.nodename, name));
        CHECK(chdir(path) == 0 && getcwd(cwd, sizeof cwd) && !strcmp(path, cwd));
        CHECK(umask(mask) == mask);
        CHECK(mount("tmpfs", path, "tmpfs", 0, NULL) == 0);
        CHECK(chdir(path) == 0);
        int f = open("private", O_CREAT | O_RDWR, 0600); CHECK(f >= 0);
        CHECK(write(f, name, strlen(name)) == (ssize_t)strlen(name));
        CHECK(close(f) == 0 && chdir("/") == 0 && umount2(path, 0) == 0);
        CHECK(setns(utsfd, CLONE_NEWUTS) == 0);
        CHECK(uname(&u) == 0 && !strcmp(u.nodename, original_name));
        CHECK(setns(netfd, CLONE_NEWNET) == 0);
        sock = socket(AF_UNIX, SOCK_DGRAM, 0); CHECK(sock >= 0);
        CHECK(bind(sock, (const struct sockaddr *)&address, address_len) == -1 && errno == EADDRINUSE);
        CHECK(close(sock) == 0);
        int alias = fcntl(utsfd, F_DUPFD, 200); CHECK(alias >= 200);
        CHECK(syscall(SYS_close_range, alias, alias, 6) == 0);
        CHECK(fcntl(alias, F_GETFD) == FD_CLOEXEC);
        CHECK(syscall(SYS_close_range, alias, alias, 0) == 0);
        CHECK(fcntl(alias, F_GETFD) == -1 && errno == EBADF);
    }
    elapsed[id] = now() - began;
    uint64_t value; CHECK(read(local, &value, sizeof value) == sizeof value && value == 7);
    CHECK(close(local) == 0); rendezvous(); CHECK(rmdir(path) == 0);
    return NULL;
}
static void pid_children(void) {
    pid_t child = fork(); CHECK(child >= 0);
    if (!child) {
        CHECK(unshare(CLONE_NEWPID) == 0);
        pid_t nested = fork(); CHECK(nested >= 0);
        if (!nested) { CHECK(getpid() == 1); _exit(0); }
        int status; CHECK(waitpid(nested, &status, 0) == nested && WIFEXITED(status) && WEXITSTATUS(status) == 0);
        _exit(0);
    }
    int status; CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "compat";
    threads = argc > 2 ? (unsigned)atoi(argv[2]) : 2;
    loops = argc > 3 ? (unsigned)atoi(argv[3]) : 16;
    CHECK(threads > 0 && threads <= 12 && loops > 0);
    CHECK(unshare(CLONE_NEWNS) == 0);
    CHECK(mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == 0);
    if (argc > 4 && !strcmp(argv[4], "filtered")) {
        struct instruction instruction = {.code = 6, .value = 0x7fff0000};
        struct program filter = {.count = 1, .instructions = &instruction};
        CHECK(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0);
        CHECK(syscall(SYS_seccomp, 1, 0, &filter) == 0);
    }
    if (!strcmp(mode, "compat")) pid_children(); else CHECK(!strcmp(mode, "loops"));
    struct utsname u; CHECK(uname(&u) == 0); memcpy(original_name, u.nodename, sizeof original_name);
    utsfd = open("/proc/self/ns/uts", O_RDONLY | O_CLOEXEC); CHECK(utsfd >= 0);
    netfd = open("/proc/self/ns/net", O_RDONLY | O_CLOEXEC); CHECK(netfd >= 0);
    int root_socket = socket(AF_UNIX, SOCK_DGRAM, 0); CHECK(root_socket >= 0);
    CHECK(bind(root_socket, (const struct sockaddr *)&address, address_len) == 0);
    sentinel = eventfd(5, 0); CHECK(sentinel >= 0);
    CHECK(mkdir("/tmp", 0777) == 0 || errno == EEXIST);
    snprintf(root, sizeof root, "/tmp/trames-task-ns-%ld", (long)getpid());
    CHECK(mkdir(root, 0700) == 0 && chdir("/") == 0); umask(022);
    CHECK(pthread_barrier_init(&barrier, NULL, threads + 1) == 0);
    pthread_t ids[12];
    for (unsigned i = 0; i < threads; i++) CHECK(pthread_create(&ids[i], NULL, worker, (void *)(uintptr_t)i) == 0);
    rendezvous(); rendezvous(); uint64_t longest = 0;
    for (unsigned i = 0; i < threads; i++) { CHECK(pthread_join(ids[i], NULL) == 0); if (elapsed[i] > longest) longest = elapsed[i]; }
    CHECK(pthread_barrier_destroy(&barrier) == 0 && rmdir(root) == 0);
    char cwd[16]; CHECK(getcwd(cwd, sizeof cwd) && !strcmp(cwd, "/") && umask(022) == 022);
    CHECK(uname(&u) == 0 && !strcmp(u.nodename, original_name));
    uint64_t value; CHECK(read(sentinel, &value, sizeof value) == sizeof value && value == 5);
    CHECK(close(sentinel) == 0 && close(utsfd) == 0 && close(netfd) == 0 && close(root_socket) == 0);
    printf("{\"mode\":\"%s\",\"threads\":%u,\"loops\":%u,\"elapsed_ns\":%llu}\n", mode, threads, loops, (unsigned long long)longest);
    return 0;
}
