#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Artem module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Message to print on load");

static int __init hello_init(void)
{
    printk(KERN_INFO "lab5: %s\n", message);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "lab5: Goodbye from Artem module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Koscov Artem");
MODULE_DESCRIPTION("Lab5: simple hello world module");
MODULE_VERSION("1.0");
