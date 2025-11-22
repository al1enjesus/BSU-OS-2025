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
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xc2fefbb5, "kmalloc_caches" },
	{ 0x38395bf3, "__kmalloc_cache_noprof" },
	{ 0x9479a1e8, "strnlen" },
	{ 0xf8d7ac5e, "proc_create" },
	{ 0xe8213e80, "_printk" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0xb9e81daf, "proc_remove" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xd272d446, "__fentry__" },
	{ 0x43a349ca, "strlen" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x70eca2ca, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0xd272d446,
	0xbd03ed67,
	0xc2fefbb5,
	0x38395bf3,
	0x9479a1e8,
	0xf8d7ac5e,
	0xe8213e80,
	0xcb8b6ec6,
	0xe54e0a6b,
	0xb9e81daf,
	0xd710adbf,
	0x092a35a2,
	0xd272d446,
	0x43a349ca,
	0xa61fd7aa,
	0x092a35a2,
	0x70eca2ca,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"__x86_return_thunk\0"
	"random_kmalloc_seed\0"
	"kmalloc_caches\0"
	"__kmalloc_cache_noprof\0"
	"strnlen\0"
	"proc_create\0"
	"_printk\0"
	"kfree\0"
	"__fortify_panic\0"
	"proc_remove\0"
	"__kmalloc_noprof\0"
	"_copy_from_user\0"
	"__fentry__\0"
	"strlen\0"
	"__check_object_size\0"
	"_copy_to_user\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "289338A81DE976E0C761360");
