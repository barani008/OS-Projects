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
	{ 0xdf61d7ba, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x754d539c, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0x152052d9, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0x65ed7360, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x586463d, __VMLINUX_SYMBOL_STR(increase) },
	{ 0xbff8afca, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0xc7917ed2, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xeaf09971, __VMLINUX_SYMBOL_STR(module_put) },
	{ 0xea6ba457, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xc7195362, __VMLINUX_SYMBOL_STR(get_addr) },
	{ 0x1c289bdd, __VMLINUX_SYMBOL_STR(get_table) },
	{ 0xaf883521, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0x5ce37b7a, __VMLINUX_SYMBOL_STR(get_vect) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x3fc32389, __VMLINUX_SYMBOL_STR(try_module_get) },
	{ 0xfb4bad77, __VMLINUX_SYMBOL_STR(reduce) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "CC4DE3B973CBD55E40E6431");
