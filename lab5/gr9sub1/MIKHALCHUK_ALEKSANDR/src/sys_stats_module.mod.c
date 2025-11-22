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
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xb9e81daf, "proc_remove" },
	{ 0xc7ffe1aa, "si_meminfo" },
	{ 0xd272d446, "__rcu_read_lock" },
	{ 0x43f4e0dd, "init_task" },
	{ 0xd272d446, "__rcu_read_unlock" },
	{ 0x058c185a, "jiffies" },
	{ 0xe199f25f, "jiffies_to_msecs" },
	{ 0x40a621c5, "snprintf" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xd272d446, "__fentry__" },
	{ 0xf8d7ac5e, "proc_create" },
	{ 0xe8213e80, "_printk" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xd272d446,
	0xb9e81daf,
	0xc7ffe1aa,
	0xd272d446,
	0x43f4e0dd,
	0xd272d446,
	0x058c185a,
	0xe199f25f,
	0x40a621c5,
	0xa61fd7aa,
	0x092a35a2,
	0xd272d446,
	0xd272d446,
	0xf8d7ac5e,
	0xe8213e80,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__x86_return_thunk\0"
	"proc_remove\0"
	"si_meminfo\0"
	"__rcu_read_lock\0"
	"init_task\0"
	"__rcu_read_unlock\0"
	"jiffies\0"
	"jiffies_to_msecs\0"
	"snprintf\0"
	"__check_object_size\0"
	"_copy_to_user\0"
	"__stack_chk_fail\0"
	"__fentry__\0"
	"proc_create\0"
	"_printk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "D8FEC3A44BD07B1B2A9A7D1");
