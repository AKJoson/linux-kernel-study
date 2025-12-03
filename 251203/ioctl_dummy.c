#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h> // register_chrdev
#include "ioctl_dummy.h"

#define IOCTL_DUMMY_DIRVER "icotl-dummy-driver"

int major = 0;

static int ioctl_dummy_open(struct inode *node,struct file *filp){
    pr_info("call-open\n");
    return 0;
}
int cache_val = 1;
//	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
static long ioctl_dummy(struct file *filp, unsigned int cmd, unsigned long arg){
    pr_info("ioctl_dummy\n");
    struct my_data data;
    switch(cmd){
        case WR_VALUE_CMD:
            pr_info("ioctl - WR_VALUE_CMD\n");
            if(copy_from_user(&cache_val,(int32_t *) arg,sizeof(cache_val))){
                pr_err("ioctl - WR_VALUE_CMD copy_from_user error!\n");
                return -1;
            }else{
                pr_info("ioctl - WR_VALUE_CMD recevie value %d\n",cache_val);
            }
            break;
        case RD_VALUE_CMD:
            pr_info("ioctl - RD_VALUE_CMD\n");
            if(copy_to_user((int32_t *) arg,&cache_val,sizeof(cache_val))){
                pr_err("ioctl - RD_VALUE_CMD copy_to_user error!\n");
                return -1;
            }else{
                pr_info("ioctl - RD_VALUE_CMD reply user.");
            }
            break;
        case WRD_VALUE_CMD:
            pr_info("ioctl - WRD_VALUE_CMD\n");
            if(copy_from_user(&data,(struct my_data*)arg,sizeof(data))){
                pr_err("ioctl - WRD_VALUE_CMD copy_from_user error!\n");
                return -1;
            }else{
                pr_info("Receive data:value is %d, data is %s\n",data.value,data.data);
                data.value = 1024;
                memcpy(data.data, "Value is from kernel.", sizeof(data.data) - 1);
                data.data[sizeof(data.data) - 1] = '\0';  // 确保以 '\0' 结尾
                if(copy_to_user((struct my_data*)arg,&data,sizeof(data))){
                    pr_err("ioctl - WRD_VALUE_CMD copy_to_user error!\n");
                    return -1;
                }else{
                    pr_info("ioctl - WRD_VALUE_CMD reply user.");
                }
            }
            break;
        default:
            pr_info("ioctl - nothing to do.\n");
    }
    return 0;
}

static struct file_operations ioctl_fops ={
    .owner = THIS_MODULE,
    .open = ioctl_dummy_open,
    .unlocked_ioctl = ioctl_dummy,
};

static int __init start_init(void){
    pr_info("start_init()\n");
    //int register_chrdev(unsigned int major, const char *name,const struct file_operations *fops)
    major = register_chrdev(0,IOCTL_DUMMY_DIRVER,&ioctl_fops);
    if(major < 0){
        pr_err("allocat major error%d\n",major);
        return major;
    }
    pr_info("major is %d\n",major);
    return 0;
}

static void __exit start_exit(void){
    pr_info("start_exit()\n");
   // void unregister_chrdev(unsigned int major, const char *name)
    unregister_chrdev(major,IOCTL_DUMMY_DIRVER);
}


module_init(start_init);
module_exit(start_exit);

MODULE_LICENSE("GPL");