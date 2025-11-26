#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wait.h>

#define VERSION "0.0.1"
#define DRIVER_NAME "cherry_driver"
#define MINOR_NUM 1
#define MSG_BUF_SIZE 10

struct msg_driver *msg_driver; //全局数据指针，用指针来增加灵活性，exit的时候需要该变量进行释放内存

struct msg_driver{
    struct cdev cdev; // 字符驱动的关键数据结构
    struct class *class;
    struct device *device;
    dev_t cherry_dev_t; //驱动号
    char data[MSG_BUF_SIZE + 1]; // +1 为\0结尾
    int size; //当前有效长度
    wait_queue_head_t read_todo,write_todo; // 读写等待队列
    struct mutex driver_mutex; // 互斥锁
};

static int cherry_open(struct inode *inode, struct file *fp){
    pr_info("cherry_open\n");
    // 根据cdev偏移量来寻找结构体的首地址，神功大法
    struct msg_driver *msg_driver = container_of(inode->i_cdev,struct msg_driver,cdev);
    fp->private_data = msg_driver;
    return 0;
}

/**
* 	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
*	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
*/
// 用户程序发来write
static ssize_t cherry_write(struct file *fp,const char __user *buf,size_t count,loff_t *offest){
    //从private_data获取结构体
    struct msg_driver *msg_driver = fp->private_data;
    // msg_driver是否有数据？ 有就进行睡眠等待，没有就进行数据写入
    if(msg_driver->size > 0){
        // 有数据
        DEFINE_WAIT(write_wait); // 主要是获取current进程变量，准备将他加入等待队列
        add_wait_queue(&msg_driver->write_todo,&write_wait);
        while(msg_driver->size > 0){
            // 准备进入睡眠状态，等条件成立再结束睡眠
            prepare_to_wait(&msg_driver->write_todo,&write_wait,TASK_INTERRUPTIBLE);
            pr_info("write prepare sleeping...\n");
            schedule();
        }
        finish_wait(&msg_driver->write_todo,&write_wait);
    }
    // 写入数据：
    mutex_lock(&msg_driver->driver_mutex);
    size_t write_count = min(count,(size_t)MSG_BUF_SIZE);
    if(copy_from_user(msg_driver->data,buf,write_count)){
        mutex_unlock(&msg_driver->driver_mutex);
        pr_err("copy_from_user error!\n");
        return -EFAULT;
    }

    msg_driver->size = write_count;
    mutex_unlock(&msg_driver->driver_mutex);
    // 唤醒等待读取的休眠进程
    pr_info("wake up read task\n");
    wake_up_interruptible(&msg_driver->read_todo);
    return write_count;

    // //1. 检查写入输入的长度
    // size_t write_count = min(count,(size_t)MSG_BUF_SIZE);
    // pr_info("Write chr count-%lu\n",write_count);
    // //2. 拷贝数据到内核，参数： 接收buf,读取地址，读取数量
    // if(copy_from_user(msg_driver->data,buf,write_count)){
    //     pr_err("copy_from_user error");
    //     return -EFAULT;
    // }
    // msg_driver->size = write_count;
    // msg_driver->data[write_count] = '\0'; // 终止符号
    //返回接收Byte数量
    // return write_count;
}

// 用户程序发来read
static ssize_t cherry_read(struct file *fp,char __user *buf,size_t count,loff_t *offset){
    struct msg_driver *msg_driver = fp->private_data;
    // 是否有数据，有就读取，没有就休眠
    if(msg_driver->size == 0){
        //没有数据
        DEFINE_WAIT(read_wait);
        // 加入队列
        add_wait_queue(&msg_driver->read_todo,&read_wait);
        while(msg_driver->size == 0){
            // 状态改变TASK_INTERRUPTIBLE
            prepare_to_wait(&msg_driver->read_todo,&read_wait,TASK_INTERRUPTIBLE);
            pr_info("read prepare to sleeping...");
            schedule();
        }
        //设置进程状态TASK_RUNNING状态，并且移除队列
        finish_wait(&msg_driver->read_todo,&read_wait);
    }
    mutex_lock(&msg_driver->driver_mutex);
    size_t read_count = min(count,(size_t)msg_driver->size);
    if(copy_to_user(buf,msg_driver->data,read_count)){
        mutex_unlock(&msg_driver->driver_mutex);
        pr_err("copyt_to_user error\n");
        return -EFAULT;
    }
    //清除数据
    msg_driver->size = 0;
    mutex_unlock(&msg_driver->driver_mutex);
    //唤醒等待写入的进程
    pr_info("wake up write task.\n");
    wake_up_interruptible(&msg_driver->write_todo);
    return read_count;

    // size_t available, read_count;

    // if (*offset >= msg_driver->size){
    //     pr_info("no more data.\n");
    //     return 0; // EOF
    // }

    // available = msg_driver->size - *offset;
    // read_count = min(count, available);

    // if (copy_to_user(buf, msg_driver->data + *offset, read_count))
    //     return -EFAULT;

    // *offset += read_count;
    // return read_count;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = cherry_open,
    .write = cherry_write,
    .read = cherry_read,
};

static int __init cherry_driver_init(void){
    int ret = 0;
    pr_info("cherry_driver_init.\n");
    //1. 初始化msg_driver kzalloc 会对数据进行清零动作，优点是没有脏数据，缺点是比kmalloc慢
    // msg_driver只在驱动进行insmod的时候创建了一次，因此所有进程共享这个数据结构体
    msg_driver = kzalloc(sizeof(struct msg_driver),GFP_KERNEL);
    msg_driver->size = 0;
    //2. 初始化等待队列
    init_waitqueue_head(&msg_driver->read_todo);
    init_waitqueue_head(&msg_driver->write_todo);
    //3. 初始化互斥锁 ，所有进程共享这把锁
    mutex_init(&msg_driver->driver_mutex);
    //初始化驱动相关的数据
    ret = alloc_chrdev_region(&msg_driver->cherry_dev_t,0,MINOR_NUM,DRIVER_NAME);
    if(ret < 0){
        pr_info("alloc dev_t error %d\n",ret);
        return ret;
    }
    // class创建
    msg_driver->class = class_create(DRIVER_NAME);
    if(IS_ERR(msg_driver->class)){
        ret = PTR_ERR(msg_driver->class);
        pr_err("Create class error- %d\n",ret);
        goto err_class;
    }

    cdev_init(&msg_driver->cdev,&fops);
    ret = cdev_add(&msg_driver->cdev,msg_driver->cherry_dev_t,1);
    if(ret < 0){
        goto err_cdev;
    }
    //device创建
    msg_driver->device = device_create(msg_driver->class,NULL,msg_driver->cherry_dev_t,NULL,DRIVER_NAME"%d",0);
    if(IS_ERR(msg_driver->device)){
        ret = PTR_ERR(msg_driver->device);
        pr_err("Unable to create device %d\n",ret);
        goto err;
    }
    return 0;    
err:
    cdev_del(&msg_driver->cdev);
err_cdev:
    class_destroy(msg_driver->class);
err_class:
    unregister_chrdev_region(msg_driver->cherry_dev_t,1);
    return ret;
}

static void __exit cherry_driver_exit(void){
    pr_info("cherry_driver_exit.\n");
    cdev_del(&msg_driver->cdev);
    device_destroy(msg_driver->class,msg_driver->cherry_dev_t);
    class_destroy(msg_driver->class);
    unregister_chrdev_region(msg_driver->cherry_dev_t,1);
    kfree(msg_driver);
}


module_init(cherry_driver_init);
module_exit(cherry_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("实现字符的读写 v" VERSION);
MODULE_VERSION(VERSION);