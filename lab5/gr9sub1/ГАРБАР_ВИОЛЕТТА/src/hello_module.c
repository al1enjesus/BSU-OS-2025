// hello_module.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>


static char *message = NULL;
module_param(message, charp, 0644);
MODULE_PARM_DESC(message, "Custom greeting message");

#define AUTHOR_NAME "Гарбар Виолетта"
#define MAX_SIZE 512  

static int __init hello_init(void) {
    if (message && message[0])
        printk(KERN_INFO "hello_module: %s\n", message);
    else
        printk(KERN_INFO "hello_module: Hello from %s module!\n", AUTHOR_NAME);

    return 0;
}

static void __exit hello_exit(void) {
    printk(KERN_INFO "hello_module: Goodbye from %s module!\n", AUTHOR_NAME);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR_NAME);
MODULE_DESCRIPTION("Simple Hello World kernel module (variant 2)");
MODULE_VERSION("1.0");

