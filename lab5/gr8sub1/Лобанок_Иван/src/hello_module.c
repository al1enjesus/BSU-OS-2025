#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);

static int __init hello_init(void) {
    if (message) {
	printk(KERN_INFO "hello_module: %s\n", message);
    }
     else {
	printk(KERN_INFO "hello_module: Hello from Ivan Lobanok module!\n");
    }
    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "hello_module: Goodbye from Ivan Lobanok module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("IVAN LOBANOK");
MODULE_DESCRIPTION("Simple Hello World my module");
MODULE_VERSION("1.0");
