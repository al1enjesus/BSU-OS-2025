#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    if (message)
        printk(KERN_INFO "hello_module: %s\n", message);
    else
        printk(KERN_INFO "hello_module: Hello from Paniavin Raman (Var 1)!\n");

    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_module: Goodbye from Paniavin Raman (Var 1)!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paniavin Raman <raman@example.com>");
MODULE_DESCRIPTION("Hello World module - Variant 1");
MODULE_VERSION("1.0");
