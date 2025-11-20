#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char* message = "Hello from Artsem Khvainitski module!";
module_param(message, charp, 0444);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    printk(KERN_INFO "hello_module: %s\n", message);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Artsem Khvainitski module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artsem Khvainitski");
MODULE_DESCRIPTION("Simple Hello World kernel module with parameter");
