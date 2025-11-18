#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shemet Alina"); 
MODULE_DESCRIPTION("Simple Hello World module with parameter");
MODULE_VERSION("1.0");

static char *message = "Hello from ШеметАА module!";
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Message to display on module load");

static int __init hello_init(void)
{
    printk(KERN_INFO "%s\n", message);
    return 0;
}
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye from ШеметАА module!\n");
}
module_init(hello_init);
module_exit(hello_exit);
