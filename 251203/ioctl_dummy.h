#ifndef ICOTL_DUMMY_H
#define IOCTL_DUMMY_H
struct my_data{
    int value;
    char data[64];
};

/*
* #define _IOR(type,nr,size)	_IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
* #define _IOW(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
* #define _IOWR(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
*/
//写命令，传递的数据是一个int32  应用程序像内核写
#define WR_VALUE_CMD _IOW('c','a',int32_t *)
//读命令，要读的数据是一个int32
#define RD_VALUE_CMD _IOR('c','b',int32_t *)

#define WRD_VALUE_CMD _IOWR('c','c',struct my_data *)

#endif