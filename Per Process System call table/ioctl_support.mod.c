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
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xbff8afca, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x5f56b8ef, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xe9a79413, __VMLINUX_SYMBOL_STR(increase_ref_count) },
	{ 0xbeb88b7, __VMLINUX_SYMBOL_STR(get_vectorid_by_address_head) },
	{ 0xc820f67a, __VMLINUX_SYMBOL_STR(reduce_ref_count) },
	{ 0x1326cc97, __VMLINUX_SYMBOL_STR(get_vector_head) },
	{ 0xafd7afc3, __VMLINUX_SYMBOL_STR(pid_task) },
	{ 0x14e0931a, __VMLINUX_SYMBOL_STR(find_vpid) },
	{ 0x65ed7360, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xb5419b40, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xea6ba457, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xdf61d7ba, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xc7917ed2, __VMLINUX_SYMBOL_STR(mutex_lock) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=vector_control";


MODULE_INFO(srcversion, "02B876B3C70912C23286B28");
