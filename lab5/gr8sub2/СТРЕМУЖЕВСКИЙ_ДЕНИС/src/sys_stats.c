#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/jiffies.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>
#include <linux/types.h>
#include <linux/math64.h>

#define PROC_NAME "sys_stats"

static struct proc_dir_entry *proc_entry;

static ssize_t sys_stats_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char kbuf[256];
	int len;
	int processes = 0;
	struct task_struct *task;
	struct sysinfo i;
	u64 used_bytes;
	unsigned long used_mb;
	unsigned long uptime_ms;
	unsigned long uptime_sec;

	rcu_read_lock();
	for_each_process(task)
		processes++;
	rcu_read_unlock();

	si_meminfo(&i);
	used_bytes = (u64)(i.totalram - i.freeram) * i.mem_unit;
	do_div(used_bytes, 1024 * 1024);
	used_mb = (unsigned long)used_bytes;

	uptime_ms = jiffies_to_msecs(jiffies);
	uptime_sec = uptime_ms / 1000;

	len = scnprintf(kbuf, sizeof(kbuf),
			"Processes: %d\n"
			"Memory Used: %lu MB\n"
			"System Uptime: %lu seconds\n",
			processes, used_mb, uptime_sec);

	return simple_read_from_buffer(buf, count, ppos, kbuf, len);
}

static const struct proc_ops sys_stats_ops = {
	.proc_read = sys_stats_read,
};

static int __init sys_stats_init(void)
{
	proc_entry = proc_create(PROC_NAME, 0444, NULL, &sys_stats_ops);
	if (!proc_entry) {
		pr_err("sys_stats: failed to create /proc/%s\n", PROC_NAME);
		return -ENOMEM;
	}

	pr_info("sys_stats: module loaded\n");
	return 0;
}

static void __exit sys_stats_exit(void)
{
	if (proc_entry)
		proc_remove(proc_entry);

	pr_info("sys_stats: module unloaded\n");
}

module_init(sys_stats_init);
module_exit(sys_stats_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Denis");
MODULE_DESCRIPTION("/proc/sys_stats system statistics module");
