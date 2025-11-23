#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROCNAME "studentinfo"
#define MAXSIZE 1024

static struct proc_dir_entry *procfile = NULL;
static int readcount = 0;
static unsigned long loadtime = 0;

static DEFINE_MUTEX(proc_mutex);

static ssize_t procread(struct file *file, char __user *ubuf, size_t count,
                        loff_t *ppos) {
  char buf[MAXSIZE];
  int len = 0;

  if (*ppos > 0)
    return 0;

  mutex_lock(&proc_mutex);
  readcount++;
  len = snprintf(buf, sizeof(buf),
                 "Name: Pardaev Sergey\nGroup: 6, Subgroup: 1\nModule loaded "
                 "at %lu jiffies\nRead count: %d\n",
                 loadtime, readcount);
  mutex_unlock(&proc_mutex);

  if (copy_to_user(ubuf, buf, len))
    return -EFAULT;
  *ppos = len;
  return len;
}

static const struct proc_ops procfileops = {
    .proc_read = procread,
};

static int __init procmoduleinit(void) {
  printk(KERN_INFO "procmodule: Initializing\n");
  loadtime = jiffies;
  procfile = proc_create(PROCNAME, 0444, NULL, &procfileops);
  if (!procfile) {
    printk(KERN_ERR "procmodule: Failed to create /proc/%s\n", PROCNAME);
    return -ENOMEM;
  }
  printk(KERN_INFO "procmodule: Created /proc/%s\n", PROCNAME);
  return 0;
}

static void __exit procmoduleexit(void) {
  if (procfile)
    proc_remove(procfile);
  printk(KERN_INFO "procmodule: Removed /proc/%s\n", PROCNAME);
  printk(KERN_INFO "procmodule: Exiting\n");
}

module_init(procmoduleinit);
module_exit(procmoduleexit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pardaev Sergey");
MODULE_DESCRIPTION("Proc filesystem example");
MODULE_VERSION("1.0");
