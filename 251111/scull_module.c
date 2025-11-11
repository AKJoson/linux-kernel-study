#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>   //register_chrdev
#include <linux/device.h> // class_create,device_create
#include <linux/cdev.h>    // cdev


MODULE_LICENSE("GPL");

#define DEVICE_NUM 4

struct my_data{
    int size;
    struct cdev scull_cdev; // 设备节点
}mydata;

static int scull_major = 0;
static struct class *scull_class; // class指针


static int my_open(struct inode *inode,struct file *file){
    pr_info("my_open\n");
    struct my_data *data;
    // 找到结构体偏移
    data =  container_of(file->private_data, struct my_data, scull_cdev);
    //赋值
    file->private_data = data;
    return 0;
}

static int my_read(struct file *file,char __user *user_buffer, size_t size,loff_t *offset){
    pr_info("my_read");
    char *str = "Nice to meet you!\n";
    int len = strlen(str);
    if(*offset >=len){ // 已经读完，返回0
        return 0; 
    }
    if(size > len - *offset){
        size = len - *offset;
    }
    
//    struct my_data *data;
//    data = file->private_data;

    copy_to_user(user_buffer,str + *offset, size);
    // 更新偏移量，否则会导致一直读取，死循环...
    *offset += size;
    return size;
}


struct file_operations file_ops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read
};

static int __init scull_init(void){
    pr_info("scull_init\n");
    dev_t dev; //内核来为我动态分配主驱动号
    int ret, i;
    // 1. 动态分配设备驱动号，完成驱动注册
    ret = alloc_chrdev_region(&dev,0,DEVICE_NUM,"ypscull");
    if(ret < 0){
        pr_err("allocate chedev region error\n");
        return ret;
    }
    scull_major = MAJOR(dev);
    pr_info("ypscull major %d\n",scull_major);
    // 2. 初始化cdev并添加到内核
    cdev_init(&mydata.scull_cdev,&file_ops);
    // 让这个scull_cdev一次关联DEVICE_NUM个节点
    ret = cdev_add(&mydata.scull_cdev,dev,DEVICE_NUM);
    if(ret){
        pr_err("failed to add cdev\n");
        return ret;
    }
    // 3. 创建类
    scull_class = class_create(THIS_MODULE,"ypscull");
    if(IS_ERR(scull_class)){
        cdev_del(&mydata.scull_cdev);
        unregister_chrdev_region(dev,DEVICE_NUM);
        return PTR_ERR(scull_class);
    }
    //4. 动态创建设备节点 /dev/ypscull0 -> /dev/ypscull3
    for(i = 0;i<DEVICE_NUM;i++){
        device_create(scull_class,NULL,MKDEV(scull_major,i),NULL,"ypscull%d",i);
    }
    return 0; 
}


static void __exit scull_exit(void){
    pr_info("scull_exit\n");
    int i;
    for(i=0;i< DEVICE_NUM;i++){
        device_destroy(scull_class,MKDEV(scull_major,i));
    }
    class_destroy(scull_class);
    cdev_del(&mydata.scull_cdev);
    unregister_chrdev_region(MKDEV(scull_major,0),DEVICE_NUM);
}


module_init(scull_init);
module_exit(scull_exit);