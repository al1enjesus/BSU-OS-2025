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
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xfc961df9, "kmalloc_caches" },
	{ 0xe18fddbc, "__kmalloc_cache_noprof" },
	{ 0x9f222e1e, "alloc_chrdev_region" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xc130cdce, "cdev_init" },
	{ 0xa5782ab4, "cdev_add" },
	{ 0x0bc5fb0d, "unregister_chrdev_region" },
	{ 0x0537a61b, "cdev_del" },
	{ 0xd272d446, "__fentry__" },
	{ 0xe8213e80, "_printk" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xd268ca91, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xbd03ed67,
	0xfc961df9,
	0xe18fddbc,
	0x9f222e1e,
	0xcb8b6ec6,
	0xc130cdce,
	0xa5782ab4,
	0x0bc5fb0d,
	0x0537a61b,
	0xd272d446,
	0xe8213e80,
	0xd272d446,
	0xa61fd7aa,
	0x092a35a2,
	0x092a35a2,
	0xd268ca91,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"alloc_chrdev_region\0"
	"kfree\0"
	"cdev_init\0"
	"cdev_add\0"
	"unregister_chrdev_region\0"
	"cdev_del\0"
	"__fentry__\0"
	"_printk\0"
	"__x86_return_thunk\0"
	"__check_object_size\0"
	"_copy_from_user\0"
	"_copy_to_user\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "6580DCA9AC732CA9EDAA452");
