#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("yinpeng first module");
MODULE_AUTHOR("YP");

static int __init start_init(void){
    pr_info("module init start\n");
    return 0;
}

static void __exit start_exit(void){
    pr_info("yp Good bye\n");
}


module_init(start_init);
module_exit(start_exit);