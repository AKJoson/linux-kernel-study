#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_PATH "/dev/cherry_driver0"

int main(){
    int fd;
    char buffer[128];
    ssize_t nbytes;

    // 1. 打开设备
    fd = open(DEVICE_PATH,O_RDONLY);
    if(fd < 0){
        printf("open error\n");
        return -1;
    }

    // 从设备读取
    printf("wait msg:\n");
    nbytes = read(fd,buffer,sizeof(buffer) -1);
    if(nbytes < 0 ){
        printf("read failed\n");
        close(fd);
        return -1;
    }

    buffer[nbytes] = '\0';

    printf("read %ld bytes: %s \n",nbytes,buffer);
    close(fd);
    return 0;
}