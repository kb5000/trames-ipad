// SPDX-License-Identifier: GPL-3.0-or-later
// Identical fixed directory work through each platform's native libc API.
#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
static double now(void) {
    struct timespec t; assert(!clock_gettime(CLOCK_MONOTONIC, &t));
    return t.tv_sec + t.tv_nsec * 1e-9;
}
static void scan(DIR *dir) {
    unsigned found = 0, entries = 0;
    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        unsigned index; char extra;
        assert(sscanf(entry->d_name, "f%u%c", &index, &extra) == 1 && index < 16);
        assert(!(found & (1u << index))); found |= 1u << index; entries++;
    }
    assert(!errno && found == 65535 && entries == 16);
}
int main(int argc, char **argv) {
    assert(argc == 4 && !strcmp(argv[2], "directory"));
    unsigned count = (unsigned)strtoul(argv[3], NULL, 10); assert(count && count <= 1000000);
    assert(!mkdir(argv[1], 0700) || errno == EEXIST);
    char path[4096];
    for (unsigned i = 0; i < 16; i++) {
        assert(snprintf(path, sizeof(path), "%s/f%u", argv[1], i) < (int)sizeof(path));
        int fd = open(path, O_CREAT | O_WRONLY, 0600); assert(fd >= 0); assert(!close(fd));
    }
    uint64_t checksum = 0; double began = now();
    for (unsigned i = 0; i < count; i++) {
        DIR *dir = opendir(argv[1]); assert(dir);
        scan(dir); rewinddir(dir); scan(dir); assert(!closedir(dir)); checksum += i + 1;
    }
    double seconds = now() - began;
    printf("{\"mode\":\"directory\",\"operations\":%u,\"seconds\":%.9f,\"checksum\":%llu}\n", count, seconds, (unsigned long long)checksum);
    return 0;
}
