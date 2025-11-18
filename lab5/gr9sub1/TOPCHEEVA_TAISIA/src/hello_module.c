#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>



static char *message = "world";
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Name to greet");

static int __init hello_init(void) {
    printk(KERN_INFO "Hello from %s module!\n", message);
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye from %s module!\n", message);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tayjie");
MODULE_DESCRIPTION("Simple Hello World module for lab5");
MODULE_VERSION("1.0");

