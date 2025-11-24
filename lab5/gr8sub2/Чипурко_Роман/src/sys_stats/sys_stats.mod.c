#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5218fe90, "single_open" },
	{ 0xd272d446, "__rcu_read_lock" },
	{ 0x2e6d1a46, "init_task" },
	{ 0xd272d446, "__rcu_read_unlock" },
	{ 0xc7ffe1aa, "si_meminfo" },
	{ 0x058c185a, "jiffies" },
	{ 0xe199f25f, "jiffies_to_msecs" },
	{ 0x12cfb334, "seq_printf" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xf6cef4e0, "proc_remove" },
	{ 0x759ba330, "remove_proc_entry" },
	{ 0xd22cd56f, "seq_read" },
	{ 0x388dee05, "seq_lseek" },
	{ 0xae030cd0, "single_release" },
	{ 0xd272d446, "__fentry__" },
	{ 0x008d4a19, "proc_create" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xd268ca91, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x5218fe90,
	0xd272d446,
	0x2e6d1a46,
	0xd272d446,
	0xc7ffe1aa,
	0x058c185a,
	0xe199f25f,
	0x12cfb334,
	0xd272d446,
	0xf6cef4e0,
	0x759ba330,
	0xd22cd56f,
	0x388dee05,
	0xae030cd0,
	0xd272d446,
	0x008d4a19,
	0xe8213e80,
	0xd272d446,
	0xd268ca91,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"single_open\0"
	"__rcu_read_lock\0"
	"init_task\0"
	"__rcu_read_unlock\0"
	"si_meminfo\0"
	"jiffies\0"
	"jiffies_to_msecs\0"
	"seq_printf\0"
	"__stack_chk_fail\0"
	"proc_remove\0"
	"remove_proc_entry\0"
	"seq_read\0"
	"seq_lseek\0"
	"single_release\0"
	"__fentry__\0"
	"proc_create\0"
	"_printk\0"
	"__x86_return_thunk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "C77677BD0DDE6C4B39C49C7");
