#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("测试模块加载,卸载");

#define YP_DRIVER_NAME "ypdriver"
#define MINOR_NUM 1

static dev_t yp_driver_dev_t; // 分配的主驱动号

struct DriverData{
    int size;
    struct class *class;
    struct cdev cdev;
}my_data;

static struct file_operations yp_fops = {
    .owner = THIS_MODULE,
    // 可先不实现 read/write
};

static int __init start_init(void){
    pr_info("start init\n");
    int ret;
    // 动态分配驱动号
    ret = alloc_chrdev_region(&yp_driver_dev_t,0,MINOR_NUM,YP_DRIVER_NAME);
    if(ret < 0){
        pr_warn("unable to allocate major\n");
        return ret;
    }
    //初始化字符设备
    cdev_init(&my_data.cdev,&yp_fops); 
    //cdev_add() - add a char device to the system
    /**
    * cdev_add() - add a char device to the system
    * @p: the cdev structure for the device
    * @dev: the first device number for which this device is responsible
    * @count: the number of consecutive minor numbers corresponding to this
    *         device
    */
    ret = cdev_add(&my_data.cdev,yp_driver_dev_t,MINOR_NUM); 
    if(ret < 0){
        pr_err("cdev add failed \n");
        unregister_chrdev_region(yp_driver_dev_t,MINOR_NUM);
        return ret;
    }
    /**
    * class_create - create a struct class structure
    * @name: pointer to a string for the name of this class.
    *
    * This is used to create a struct class pointer that can then be used
    * in calls to device_create().
    *
    * Returns &struct class pointer on success, or ERR_PTR() on error.
    *
    * Note, the pointer created here is to be destroyed when finished by
    * making a call to class_destroy().
    */
    my_data.class = class_create(YP_DRIVER_NAME);
    dev_t devno;
    devno = MKDEV(MAJOR(yp_driver_dev_t),0);
    struct device *device;
    device = device_create(my_data.class,NULL,devno,NULL,"ypdriver%d",MINOR(devno));
    if(IS_ERR(device)){
        ret = PTR_ERR(device);
        pr_err("Unable to create coproc %d %d\n",MINOR(devno),ret);
        // 清空资源
        cdev_del(&my_data.cdev);
        class_destroy(my_data.class);
        unregister_chrdev_region(yp_driver_dev_t,1);
        return ret;
    }
    pr_info("yp driver init success!\n");
    return 0;
}

static void __exit start_exit(void){
    pr_info("start exit\n");
    dev_t devno;
    cdev_del(&my_data.cdev);
    devno = MKDEV(MAJOR(yp_driver_dev_t),0);
    device_destroy(my_data.class,devno);
    class_destroy(my_data.class);
    unregister_chrdev_region(yp_driver_dev_t,1);
    pr_info("start exit success!\n");
}


module_init(start_init);
module_exit(start_exit);


