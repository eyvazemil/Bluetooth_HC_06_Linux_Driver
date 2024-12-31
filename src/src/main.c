#include "ftdi_usb_driver.h"
#include "custom_macros.h"

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/errno.h>

// --------------------------------------
// Declaration of module parameters.
// --------------------------------------

/**
 * Module parameter: name of the module that will be provided to 
 * `alloc_chrdev_region()` function, as the name of this driver.
 */
static char * g_module_name = "emil_bluetooth_hc_06_driver";

/**
 * Name of the class of devices that will be created in sysfs.
 * Once the `create_device_class()` is called with this name in our module's
 * initialization function, we will see this name in `/sys/class`.
 */
static char * g_device_class_name = "emil_hc_06";

/**
 * Maximum packet size of the USB interface bulk in/out endpoints, that will be 
 * used to read the exact number of bytes from that endpoint into our buffer.
 */
static int g_usb_bulk_endpoint_max_packet_size = 64;

/**
 * Permission `S_IRUGO` means that the world can see the value of this parameter,
 * but can't change it, where as `S_IRUGO | S_IWUSR` means that only root can change
 * the parameter.
 */
module_param(g_module_name, charp, S_IRUGO);
module_param(g_device_class_name, charp, S_IRUGO);
module_param(g_usb_bulk_endpoint_max_packet_size, int, S_IRUGO);

// --------------------------------------------
// Initialization and unitialization functions.
// --------------------------------------------

/**
 * @brief Initialization function, which will be called by `module_init()`.
 *		  It must be declared as `static`, so that it couldn't be accessible
 *		  from other files.
 * 		  It has `__init` token, before the name of the function, to let the
 *		  compiler know that it is used only for initialization and, once the
 *		  module is loaded, its memory will be freed up for other uses. 
 */
static int __init my_driver_init(void) {
	PRINT_DEBUG("__INIT__ module %s>> (kernel version %x, i.e. major: %d, \
patch-level: %d, sub-level: %d)\n", 
		g_module_name, LINUX_VERSION_CODE, LINUX_VERSION_MAJOR, 
		LINUX_VERSION_PATCHLEVEL, LINUX_VERSION_SUBLEVEL
	);

	if(g_usb_bulk_endpoint_max_packet_size <= 0) {
		PRINT_DEBUG("__INIT__ module %s>> invalid value of USB bulk endpoint max size (should be > 0): %d.\n",
			g_module_name, g_usb_bulk_endpoint_max_packet_size
		);
	}

	// Register FTDI USB device.
	int usb_registration_status = ftdi_usb_driver_register(
		g_device_class_name, g_usb_bulk_endpoint_max_packet_size
	);

	if(usb_registration_status) {
		PRINT_DEBUG("__INIT__ module %s>> failed to register USB device with error: %d\n", 
			g_module_name, usb_registration_status
		);

		return usb_registration_status;
	}

	return 0;
}

/**
 * @brief Exit function, which is called via `module_exit()`.
 *		  This function must be static.
 */
static void __exit my_driver_exit(void) {
	ftdi_usb_driver_deregister();
	PRINT_DEBUG("__EXIT__ module %s\n", g_module_name);
}

module_init(my_driver_init);
module_exit(my_driver_exit);

// ----------------------------------------------------------------------
// Declaration of license, author, and description of this kernel module.
// ----------------------------------------------------------------------

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emil Eyvazov");
MODULE_DESCRIPTION("Bluetooth slave device module for sending data via bluetooth to this machine");
