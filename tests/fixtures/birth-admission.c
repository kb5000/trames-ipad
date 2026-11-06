// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static void put(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    assert(fd >= 0);
    assert(write(fd, value, strlen(value)) == (ssize_t)strlen(value));
    assert(close(fd) == 0);
}
static void waited(pid_t pid) {
    int status;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
int main(int argc, char **argv) {
    int old_kernel_oracle = argc == 2 && !strcmp(argv[1], "--old-kernel-oracle");
    char group[128], limit[160], members[160];
    snprintf(group, sizeof group, "/sys/fs/cgroup/trames-birth-%d", getpid());
    snprintf(limit, sizeof limit, "%s/pids.max", group);
    snprintf(members, sizeof members, "%s/cgroup.procs", group);
    put("/sys/fs/cgroup/cgroup.subtree_control", "+pids");
    assert(mkdir(group, 0755) == 0);
    put(limit, "1");
    pid_t parent = fork();
    assert(parent >= 0);
    if (!parent) {
        put(members, "0");
        assert(unshare(CLONE_NEWPID) == 0);
        for (int i = 0; i < 3; ++i) {
            errno = 0;
            pid_t denied = fork();
            // Before Linux's 2026 "make sub-init creation retryable" change,
            // the first late fork failure disables this namespace permanently.
            // Record that oracle difference; guest acceptance never permits it.
            if (old_kernel_oracle && i == 1 && denied == -1 && errno == ENOMEM)
                _exit(77);
            assert(denied == -1 && errno == EAGAIN);
        }
        put(limit, "2");
        pid_t init = fork();
        assert(init >= 0);
        if (!init) {
            assert(getpid() == 1);
            _exit(0);
        }
        waited(init);
        errno = 0;
        assert(fork() == -1 && errno == ENOMEM);
        _exit(0);
    }
    int status;
    assert(waitpid(parent, &status, 0) == parent);
    assert(rmdir(group) == 0);
    if (old_kernel_oracle && WIFEXITED(status) && WEXITSTATUS(status) == 77) {
        puts("{\"case\":\"birth-admission\",\"result\":\"legacy-init-disabled\"}");
        return 77;
    }
    assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    puts("{\"case\":\"birth-admission\",\"passed\":true}");
    return 0;
}
