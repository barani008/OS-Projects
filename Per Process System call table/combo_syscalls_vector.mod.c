#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xece2da27, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6f9e0ae2, __VMLINUX_SYMBOL_STR(deregister_vector) },
	{ 0x77ddad0a, __VMLINUX_SYMBOL_STR(register_vector) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x983661cd, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0x1e6d26a8, __VMLINUX_SYMBOL_STR(strstr) },
	{ 0xb5419b40, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xea6ba457, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xdf61d7ba, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=vector_control";


MODULE_INFO(srcversion, "F81AEC2620BDC5FEC9CA82C");
