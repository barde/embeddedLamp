#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
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
	{ 0x771b4d74, "module_layout" },
	{ 0x3ec8886f, "param_ops_int" },
	{ 0x7e98d25f, "device_unregister" },
	{ 0xfe990052, "gpio_free" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xcb12d028, "__class_create" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0xfe4ecb3, "cdev_del" },
	{ 0x9ccc19d4, "class_destroy" },
	{ 0x543aaebd, "device_destroy" },
	{ 0x536222ac, "hrtimer_init" },
	{ 0x37a0cba, "kfree" },
	{ 0x55860f7c, "driver_unregister" },
	{ 0x40c93a53, "spi_add_device" },
	{ 0x73e20c1c, "strlcpy" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x5d63a5df, "bus_find_device_by_name" },
	{ 0xb81960ca, "snprintf" },
	{ 0x8301eeda, "put_device" },
	{ 0x99a7719b, "spi_alloc_device" },
	{ 0x482ed489, "spi_busnum_to_master" },
	{ 0xf5913867, "spi_register_driver" },
	{ 0xa8f59416, "gpio_direction_output" },
	{ 0xa6c30a1a, "device_create" },
	{ 0xd06f7f55, "cdev_add" },
	{ 0xac6e38ed, "cdev_init" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x2196324, "__aeabi_idiv" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x91715312, "sprintf" },
	{ 0x2d4c3353, "hrtimer_forward" },
	{ 0x432fd7f6, "__gpio_set_value" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xb742fd7, "simple_strtol" },
	{ 0x5f754e5a, "memset" },
	{ 0xb99492d7, "hrtimer_start" },
	{ 0x4e830a3e, "strnicmp" },
	{ 0x97255bdf, "strlen" },
	{ 0x27e1a049, "printk" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0xd901b4cb, "spi_async" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xc42b953, "kmalloc_caches" },
	{ 0x2eb22412, "up" },
	{ 0xd7c0a9fb, "kmem_cache_alloc_trace" },
	{ 0x7cafcf7a, "down_interruptible" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x74c97f9c, "_raw_spin_unlock_irqrestore" },
	{ 0xbd7083bc, "_raw_spin_lock_irqsave" },
	{ 0xc084b53a, "hrtimer_cancel" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "D6921758C42D92DCF18643E");
