#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ioctl_dummy.h"

int main(){
    int fd = open("/dev/ioctl-dumpy",O_WRONLY | O_RDONLY);
    if(fd < 0){
        printf("open /dev/ioctl-dumpy failed!\n");
        return fd;
    }

    int answer = 10;
    struct my_data data = {1024,"hello world!"};
    ioctl(fd,WR_VALUE_CMD,&answer);
    ioctl(fd,RD_VALUE_CMD,&answer);
    printf("receiver Reply %d\n",answer);
    ioctl(fd,WRD_VALUE_CMD,&data);
    printf("receive Reply %d, %s\n",data.value,data.data);
    return 0;
}