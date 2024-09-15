#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#define BUG_ON(cond) \
    do { \
        if (cond) { \
            fprintf(stderr, "BUG_ON: %s (L%d) %s\n", __FILE__, __LINE__, __FUNCTION__); \
            raise(SIGABRT); \
        } \
    } while (0)

using namespace std;


int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 2) {
        printf("Usage: %s <path> <number of points>\n", argv[0]);
        return 1;
    }

    const char *path = argv[1];
    uint32_t num_points = 0;

    int fd = open(path, O_RDWR);
    BUG_ON(fd < 0);

    ssize_t ret = pread(fd, &num_points, sizeof(num_points), 0);
    BUG_ON(ret != sizeof(num_points));
    printf("Current number of points: %d\n", num_points);

    if (argc == 2)
        return 0;

    num_points = atoi(argv[2]);
    BUG_ON(num_points <= 0);

    ret = pwrite(fd, &num_points, sizeof(num_points), 0);
    BUG_ON(ret != sizeof(num_points));
    printf("Number of points changed to: %d\n", num_points);

    return 0;
}
