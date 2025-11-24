#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bogdevich Artem");
MODULE_DESCRIPTION("Simple Hello World module for lab5");
MODULE_VERSION("1.0");

static char *message = "Hello from БОГДЕВИЧ_АР module!";
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom message to display");

static int __init hello_init(void)
{
    printk(KERN_INFO "=== HELLO_MODULE: %s\n", message);
    return 0;
}

static void __exit hello_exit(void)
{
    printk(KERN_INFO "=== HELLO_MODULE: Goodbye from БОГДЕВИЧ_АР module!\n");
}

module_init(hello_init);
module_exit(hello_exit);
