#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Matvey Gorbach module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Greeting message");

static int __init hello_init(void) {
	printk(KERN_INFO "hello_module: %s\n", message);
	return 0;
}

static void __exit hello_exit(void) {
	printk(KERN_INFO "hello_module: Goodbye from module\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matvey Gorbach");
MODULE_DESCRIPTION("Simple Hello World kernerl module for lab 5");
MODULE_VERSION("0.1");
