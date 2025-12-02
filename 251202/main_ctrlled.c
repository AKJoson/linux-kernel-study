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
    int value = 0;
    char on[1] = {'1'};
    char off[1] = {'0'};
    while(1){
        if(value){
            write(fd,off,1);
        }else{
            write(fd,on,1);
        }
        // 切换LED状态
        value = !value;
        sleep(1);
    }

    // 关闭文件描述符
    close(fd);
    return 0;
}