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
	{ 0xf46d5bf3, "mutex_lock" },
	{ 0x40a621c5, "snprintf" },
	{ 0xf46d5bf3, "mutex_unlock" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0xf6cef4e0, "proc_remove" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xd272d446, "__fentry__" },
	{ 0x008d4a19, "proc_create" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xd268ca91, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xf46d5bf3,
	0x40a621c5,
	0xf46d5bf3,
	0xa61fd7aa,
	0x092a35a2,
	0xd272d446,
	0xf6cef4e0,
	0x092a35a2,
	0x90a48d82,
	0xd272d446,
	0x008d4a19,
	0xe8213e80,
	0xd272d446,
	0xd268ca91,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"mutex_lock\0"
	"snprintf\0"
	"mutex_unlock\0"
	"__check_object_size\0"
	"_copy_to_user\0"
	"__stack_chk_fail\0"
	"proc_remove\0"
	"_copy_from_user\0"
	"__ubsan_handle_out_of_bounds\0"
	"__fentry__\0"
	"proc_create\0"
	"_printk\0"
	"__x86_return_thunk\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "EEFE55B2E1B663B9BA2DCE7");
