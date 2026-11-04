// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s errno=%d\n", __LINE__, #x, errno); exit(1); } } while (0)
static int ready[2], resume_child[2];

static void *check_thread(void *unused) {
    (void)unused;
    pid_t tid = (pid_t)syscall(SYS_gettid);
    CHECK(write(ready[1], &tid, sizeof tid) == sizeof tid);
    char byte;
    CHECK(read(resume_child[0], &byte, 1) == 1);
    struct rlimit limit;
    CHECK(prlimit(tid, RLIMIT_NOFILE, NULL, &limit) == 0);
    CHECK(limit.rlim_cur == 16 && limit.rlim_max == 32);
    CHECK(getrlimit(RLIMIT_FSIZE, &limit) == 0 && limit.rlim_cur == 7);
    int fds[32], count = 0;
    while (count < 32) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) { CHECK(errno == EMFILE); break; }
        CHECK(fd < 16);
        fds[count++] = fd;
    }
    CHECK(count > 0 && count < 32);
    while (count) CHECK(close(fds[--count]) == 0);
    return NULL;
}

int main(void) {
    struct rlimit original, size, old;
    CHECK(getrlimit(RLIMIT_NOFILE, &original) == 0);
    CHECK(getrlimit(RLIMIT_FSIZE, &size) == 0);
    CHECK(pipe(ready) == 0 && pipe(resume_child) == 0);
    pid_t child = fork();
    CHECK(child >= 0);
    if (!child) {
        close(ready[0]); close(resume_child[1]);
        pthread_t thread;
        CHECK(pthread_create(&thread, NULL, check_thread, NULL) == 0);
        CHECK(pthread_join(thread, NULL) == 0);
        CHECK(getrlimit(RLIMIT_NOFILE, &old) == 0);
        CHECK(old.rlim_cur == 16 && old.rlim_max == 32);
        _exit(0);
    }
    close(ready[1]); close(resume_child[0]);
    pid_t tid;
    CHECK(read(ready[0], &tid, sizeof tid) == sizeof tid);
    struct rlimit limit = {16, 32};
    CHECK(prlimit(tid, RLIMIT_NOFILE, &limit, &old) == 0);
    CHECK(old.rlim_cur == original.rlim_cur && old.rlim_max == original.rlim_max);
    limit = size; limit.rlim_cur = 7;
    CHECK(prlimit(child, RLIMIT_FSIZE, &limit, &old) == 0);
    CHECK(old.rlim_cur == size.rlim_cur);
    CHECK(getrlimit(RLIMIT_NOFILE, &old) == 0 && old.rlim_cur == original.rlim_cur);
    CHECK(getrlimit(RLIMIT_FSIZE, &old) == 0 && old.rlim_cur == size.rlim_cur);
    CHECK(write(resume_child[1], "x", 1) == 1);
    int status;
    CHECK(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    CHECK(prlimit(child, RLIMIT_NOFILE, NULL, &old) == -1 && errno == ESRCH);
    close(ready[0]); close(resume_child[1]);
    puts("prlimit-cross-process-ok");
    return 0;
}
