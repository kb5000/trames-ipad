// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main(void) {
    alarm(25);
    if (setgid(1000) || setuid(1000)) { perror("system UID"); return 1; }
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un address = {.sun_family = AF_UNIX};
    strcpy(address.sun_path, "/dev/socket/zygote");
    if (fd < 0 || connect(fd, (void *)&address, sizeof(address))) {
        perror("original zygote"); return 1;
    }
    const char *args = "9\n--runtime-args\n--setuid=1000\n--setgid=1000\n"
        "--target-sdk-version=33\n--runtime-flags=0\n"
        "--nice-name=trames-memory-stress\nAndroidMemoryStress\n3000\nexit\n";
    if (write(fd, args, strlen(args)) != (ssize_t)strlen(args)) return 1;
    unsigned char reply[5];
    for (size_t done = 0; done != sizeof(reply);) {
        ssize_t count = read(fd, reply + done, sizeof(reply) - done);
        if (count <= 0) { perror("original zygote reply"); return 1; }
        done += count;
    }
    uint32_t network_pid;
    memcpy(&network_pid, reply, sizeof(network_pid));
    int pid = (int)ntohl(network_pid);
    printf("ZYGOTE_CHILD %d wrapper=%d\n", pid, reply[4]);
    close(fd);
    return pid > 0 ? 0 : 1;
}
