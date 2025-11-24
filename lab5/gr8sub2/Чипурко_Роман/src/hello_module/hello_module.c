#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from YOUR_NAME module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Greeting message printed on module load/unload");

static int __init hello_init(void)
{
    if (message && message[0])
        printk(KERN_INFO "%s\n", message);
    else
        printk(KERN_INFO "Hello from module (no message provided)\n");

    return 0;
}

static void __exit hello_exit(void)
{
    if (message && message[0])
        printk(KERN_INFO "Goodbye from module! Last message: %s\n", message);
    else
        printk(KERN_INFO "Goodbye from module (no message provided)\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR_NAME");
MODULE_DESCRIPTION("Simple Hello World kernel module with 'message' parameter");
MODULE_VERSION("1.0");
