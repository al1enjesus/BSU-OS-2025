// hello_module.c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>

#define DRIVER_NAME "hello_module"

static char *message = NULL;
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Custom greeting message");

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ярослав");
MODULE_DESCRIPTION("Simple Hello World kernel module that accepts a message parameter");

static int __init hello_init(void)
{
    if (message && message[0]) {
        printk(KERN_INFO DRIVER_NAME ": %s\n", message);
    } else {
        printk(KERN_INFO DRIVER_NAME ": Hello from Ярослав module!\n");
    }
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO DRIVER_NAME ": Goodbye from Ярослав module!\n");
}

module_init(hello_init);
module_exit(hello_exit);
