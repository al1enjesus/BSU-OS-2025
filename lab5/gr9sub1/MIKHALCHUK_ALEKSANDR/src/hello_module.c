#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Sasha module";
module_param(message, charp, 0644);

static int __init hello_init(void) {
    printk(KERN_INFO "%s\n", message);
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye from Sasha module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sasha");
MODULE_DESCRIPTION("Simple Hello World");
MODULE_VERSION("1.0");
