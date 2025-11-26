#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

static char *message = "Hello from Vladimir module!";
module_param(message, charp, 0000);
MODULE_PARM_DESC(message, "Custom greeting message");

static int __init hello_init(void)
{
    printk(KERN_INFO "%s\n", message);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye from Vladimir module!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vladimir");
MODULE_DESCRIPTION("Simple Hello World module with parameter");
