#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef SYS_renameat2
int main(void) {
    /* Architectures without the syscall are handled by etcfs as unsupported. */
    return 77;
}
#else
static int renameat2_call(int olddirfd, const char *oldpath, int newdirfd,
                          const char *newpath, unsigned int flags) {
    return syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}

static int write_file(int dirfd, const char *name, const char *value) {
    int fd = openat(dirfd, name, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd < 0) return -1;
    size_t len = strlen(value);
    ssize_t written = write(fd, value, len);
    int saved = errno;
    close(fd);
    errno = saved;
    return written == (ssize_t)len ? 0 : -1;
}

static int read_file(int dirfd, const char *name, char *buf, size_t size) {
    int fd = openat(dirfd, name, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t got = read(fd, buf, size - 1);
    int saved = errno;
    close(fd);
    errno = saved;
    if (got < 0) return -1;
    buf[got] = '\0';
    return 0;
}

int main(void) {
    char path[] = "/tmp/bedrock-renameat2-XXXXXX";
    int dirfd = mkstemp(path);
    if (dirfd < 0) return 1;
    close(dirfd);
    unlink(path);
    if (mkdir(path, 0700) < 0) return 1;
    dirfd = open(path, O_DIRECTORY | O_RDONLY);
    if (dirfd < 0) return 1;

    int rc = 1;
    if (write_file(dirfd, "left", "L") < 0 || write_file(dirfd, "right", "R") < 0)
        goto cleanup;
    errno = 0;
    if (renameat2_call(dirfd, "left", dirfd, "right", RENAME_NOREPLACE) != -1 || errno != EEXIST)
        goto cleanup;
    if (renameat2_call(dirfd, "left", dirfd, "right", RENAME_EXCHANGE) < 0)
        goto cleanup;
    char left[2], right[2];
    if (read_file(dirfd, "left", left, sizeof(left)) < 0 || read_file(dirfd, "right", right, sizeof(right)) < 0)
        goto cleanup;
    if (strcmp(left, "R") != 0 || strcmp(right, "L") != 0)
        goto cleanup;
    rc = 0;

cleanup:
    unlinkat(dirfd, "left", 0);
    unlinkat(dirfd, "right", 0);
    close(dirfd);
    rmdir(path);
    return rc;
}
#endif
