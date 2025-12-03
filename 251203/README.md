### IOCTL的使用

* 定义cmd的关键宏
```c
#define _IOR(type,nr,size)	_IOC(_IOC_READ,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOW(type,nr,size)	_IOC(_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
#define _IOWR(type,nr,size)	_IOC(_IOC_READ|_IOC_WRITE,(type),(nr),(_IOC_TYPECHECK(size)))
```
* 应用程序端

![用户输输出](../img/ioctl_user.png)

* 内核端

![内核输出](../img/ioctl_kernel.png)