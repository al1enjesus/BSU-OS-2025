#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init chardev_init(void)
{
    printk(KERN_INFO "char_device: stub module loaded\n");
    return 0;
}

static void __exit chardev_exit(void)
{
    printk(KERN_INFO "char_device: stub module unloaded\n");
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artsem Khvainitski");
MODULE_DESCRIPTION("Stub char device module for lab structure");
