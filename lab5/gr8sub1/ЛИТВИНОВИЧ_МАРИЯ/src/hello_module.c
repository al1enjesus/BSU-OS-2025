#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static char *message = "Hello from MARIYA module!";
module_param(message, charp, 0000);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    printk(KERN_INFO "%s\n", message);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye from MARIYA module!\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MARIYA");
MODULE_DESCRIPTION("Simple Hello World kernel module");

module_init(hello_init);
module_exit(hello_exit);
