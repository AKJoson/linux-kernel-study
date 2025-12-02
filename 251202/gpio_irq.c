#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DRIVER_NAME  "ypgpioctrl"
#define TOUCH_DEBOUNCE 50

static struct gpio_desc *led;
static struct gpio_desc *touch_button;

static struct my_data{
    dev_t major;
    struct class *class;
    struct device *device;
    wait_queue_head_t todo;
    struct cdev cdev;
}*my_data;

static irqreturn_t irq_handle(int irq,void *dev_id){
    pr_info("call - irq_handle\n");
    static unsigned long last_jiffies = 0;
    if(time_after(jiffies,last_jiffies+msecs_to_jiffies(TOUCH_DEBOUNCE))){
        last_jiffies = jiffies;
        int cur_status = gpiod_get_value(led);
        if(cur_status){
            gpiod_set_value(led,0);
        }else{
            gpiod_set_value(led,1);
        }
        // 唤醒
        wake_up_interruptible(&my_data->todo);
    }
    return IRQ_HANDLED;
}

static int my_open(struct inode *node, struct file *filp){
    pr_info("call - my_open\n");
    filp->private_data = container_of(node->i_cdev,struct my_data,cdev);
    return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset){
    pr_info("call - my_write\n");
    char data[1];
    if(copy_from_user(data,buf,1)){
        pr_err("call - my_write,copy_from_user error!\n");
        return -1;
    }
    switch(data[0]){
        case '0':
        case '1':
            int status = data[0] - '0';
            gpiod_set_value(led,status);
            break;
        default:
            pr_err("call - my_write ,sign error:%s\n",data);
    }
    return count;
}

static unsigned int my_poll(struct file * filp, struct poll_table_struct *poll_table){
    pr_info("call - my_poll\n");
    poll_wait(filp,&my_data->todo,poll_table);
    int cur_val = gpiod_get_value(led);
    pr_info("cur led value:%d\n",cur_val);
    if(cur_val){
        //注意: 只返回POLLOUT，应用程序会出现一直阻塞状态
        return  POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
    }
    /* 进入阻塞状态*/
    return 0;
}

static struct file_operations my_file_ops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .write = my_write,
    .poll = my_poll,
};

static int call_probe(struct platform_device *ofdev){
    pr_info("call_probe, match!\n");
    int ret;
    struct device *dev = &ofdev->dev;
    if(!of_property_present(dev->of_node,"label")){
        pr_err("can't found label!\n");
        return -1;
    }
    const char *label_value;
    ret = of_property_read_string(dev->of_node,"label",&label_value); // 第三个参数是二级指针
    if(ret){
        pr_err("can't read label!\n");
        return -1;
    }
    pr_info("call_probe,label value is: %s\n",label_value);
    //初始化gpio
    led = gpiod_get(dev,"led",GPIOD_OUT_LOW);
    if(IS_ERR(led)){
        pr_err("can't init the led gpio!%ld\n",PTR_ERR(led));
        return PTR_ERR(led);
    }
    touch_button = gpiod_get(dev,"touch",GPIOD_OUT_HIGH);
    if(IS_ERR(touch_button)){
        pr_err("can't init the touch_button gpio!%ld\n",PTR_ERR(touch_button));
        return PTR_ERR(touch_button);
    }
    //注册中断
    /*获取中断号 */
    int irq = gpiod_to_irq(touch_button);
    if(irq < 0){
        pr_err("can't get irq value \n");
        return irq;
    }
    /*注册中断 */
    ret = devm_request_irq(dev,irq,irq_handle,IRQF_TRIGGER_FALLING,"touch_button_irq",dev);
    if(ret){
        pr_err("requet irq error!\n");
        return ret;
    }
    //注册主设备号，创建节点
    my_data = kmalloc(sizeof(struct my_data),GFP_KERNEL);
    init_waitqueue_head(&my_data->todo);
    ret = alloc_chrdev_region(&my_data->major,0,1,DRIVER_NAME);
    if(ret < 0){
        pr_err("alloc_chrdev_region error \n");
        return ret;
    }

    my_data->class = class_create(DRIVER_NAME);
    if(IS_ERR(my_data->class)){
        pr_err("can't create class\n");
        ret = PTR_ERR(my_data->class);
        goto err_class;
    }
    cdev_init(&my_data->cdev,&my_file_ops);
    ret = cdev_add(&my_data->cdev,my_data->major,1);
    if(ret < 0){
        pr_err("cdev_add error\n");
        goto err_cdev;
    }
    //device创建
    my_data->device = device_create(my_data->class,NULL,my_data->major,NULL,DRIVER_NAME"%d",0);
    if(IS_ERR(my_data->device)){
        ret = PTR_ERR(my_data->device);
        pr_err("Unable to create device %d\n",ret);
        goto err;
    }
    return 0;
err:
    cdev_del(&my_data->cdev);
err_cdev:
    class_destroy(my_data->class);
err_class:
    unregister_chrdev_region(my_data->major,1);
    return ret;
}

static void call_remove(struct platform_device *ofdev){
    pr_info("call_remove\n");

    // 清理GPIO资源
    if (led) gpiod_put(led);
    if (touch_button) gpiod_put(touch_button);

    // 卸载字符设备
    cdev_del(&my_data->cdev);

    // 销毁设备节点
    device_destroy(my_data->class, my_data->major);

    // 销毁设备类
    class_destroy(my_data->class);

    // 注销设备号
    unregister_chrdev_region(my_data->major, 1);

    // 如果存在其他内存或资源，需要手动释放
    if (my_data) {
        kfree(my_data);
    }
}

static const struct of_device_id dev_match_table[] = {
    {.compatible = "yp,touch_button_ctrl"},
    {}
};

static struct platform_driver my_gpio_driver= {
    .driver = {
        .name = "my_gpio_test",
        .of_match_table = dev_match_table,
    },
    .probe = call_probe,
    .remove = call_remove,
};

static int __init start_init(void){
    pr_info("call - start_init()\n");
    int ret = platform_driver_register(&my_gpio_driver);
    return ret;
}

static void __exit start_exit(void){
    pr_info("call - start_exit()\n");
    platform_driver_unregister(&my_gpio_driver);
}


module_init(start_init);
module_exit(start_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cherry");
MODULE_DESCRIPTION("A sample use GPIO & interrupt & POLL & IOCTL");