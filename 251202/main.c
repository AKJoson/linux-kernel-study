#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <errno.h>

int main() {
    // 打开设备文件
    int fd = open("/dev/ypgpioctrl0", O_WRONLY);
    if (fd < 0) {
        perror("open failed");
        return fd;
    }

    // 设置 pollfd 结构体
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;  // 监听是否有可以输入
    pfd.revents = 0;
        // 使用 poll 进行事件监控
        int ret = poll(&pfd, 1, -1);  // -1表示无限等待 0表示立刻返回
        if (ret < 0) {
            perror("poll failed");
            close(fd);
            return ret;
        }

        if (ret == 0) {
            printf("poll timed out\n");
        } else {
            if (pfd.revents & POLLOUT) {
                printf("led is turn on\n");
            } else {
                printf("Unexpected event\n");
            }
        }
    // 关闭文件描述符
    close(fd);
    return 0;
}