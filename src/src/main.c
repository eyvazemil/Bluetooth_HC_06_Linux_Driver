#include "char_device_registration.h"
#include "custom_macros.h"
#include "device_file_operations.h"
#include "device_data.h"

/** Header that contains initialization functions of the modules. */
#include <linux/init.h>

/**
 * Header that contains many definitions of functions and symbols
 * needed by the loadable modules.
 */
#include <linux/module.h>

/** Header that contains a Linux version information. */
#include <linux/version.h>

/** Header that conains functions for registering module parameters. */
#include <linux/moduleparam.h>

/** Header that contains `dev_t` type for module major and minor numbers. */
#include <linux/types.h>

/** Header that contains `MAJOR()` and `MINOR()` macros for extraction of
 * major and minor numbers from `dev_t` variable.
 */
#include <linux/kdev_t.h>

/** Header with `file_operations` structure that we supply our functions
 * with in order for the user to be able to make system calls that will
 * call our functions.
 */
#include <linux/fs.h>

/** Header with declaration of error values. */
#include <linux/errno.h>

/**
 * Declare the license, as without it, the kernel 
 * will complain about an absent license.
 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emil Eyvazov");
MODULE_DESCRIPTION("Bluetooth slave device module for sending data via bluetooth to this machine");

// --------------------------------------
// Declaration of module parameters.
// --------------------------------------

/**
 * Module parameter: name of the module that will be provided to 
 * `alloc_chrdev_region()` function, as the name of this driver.
 */
static char * g_module_name = "emil_bluetooth_driver";

/**
 * Name of the class of devices that will be created in sysfs.
 * Once the `create_device_class()` is called with this name in our module's
 * initialization function, we will see this name in `/sys/class`.
 */
static char * g_device_class_name = "emil_bluetooth_driver_devices_class";

/**
 * Permission `S_IRUGO` means that the world can see the value of this parameter,
 * but can't change it, where as `S_IRUGO | S_IWUSR` means that only root can change
 * the parameter.
 */
module_param(g_module_name, charp, S_IRUGO);
module_param(g_device_class_name, charp, S_IRUGO);

/** We have only one device. */
static int g_num_devices = 1;

// --------------------------------------------
// Initialization and unitialization functions.
// --------------------------------------------

/**
 * Device data for storing data from read and write file operations.
 */
struct device_data * g_device_data = NULL;

/**
 * @brief Clean up function, which is called in module unitialization
 * function and in module initialization function, when any error occurs
 * and everything has to be cleaned up.
 */
static void exit_cleanup(void) {
	if(g_device_data) {
		// Uninitialize this device only if the device data was successfully allocated.
		if(g_device_data->m_device_buffer) {
            // Unitialize this device if the device buffer was 
            // successfully allocated.
            destroy_sysfs_device();
            uninitialize_device(&(g_device_data->m_cdev));
            kfree(g_device_data->m_device_buffer);
        }

		kfree(g_device_data);
	}

	// Destroy the class of devices that were registered in sysfs.
	destroy_device_class();

	// Unregister the devices associated with this driver.
	unregister_driver_number();
}

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
patch-level: %d, sub-level: %d), number of devices: %d\n", 
		g_module_name, LINUX_VERSION_CODE, LINUX_VERSION_MAJOR, 
		LINUX_VERSION_PATCHLEVEL, LINUX_VERSION_SUBLEVEL, g_num_devices
	);

	// Register the major and minor numbers of our driver.
	int register_driver_number_result = register_driver_number(g_module_name);
	
	if(register_driver_number_result < 0) {
        PRINT_DEBUG("register_driver_number() has failed!\n");
        return register_driver_number_result;
    }

	dev_t major_minor_numbers = get_driver_number();

	PRINT_DEBUG("__INIT__ module %s>> major number: %d, minor number: %d\n",
		g_module_name, MAJOR(major_minor_numbers), 
		MINOR(major_minor_numbers)
	);

	// Create a class for the devices that we will create in sysfs, i.e. in `/dev/`.
	int create_device_class_result = create_device_class(g_device_class_name);

	if(create_device_class_result) {
		exit_cleanup();
		return -ENODEV;
	}

	// Allocate device data.
	g_device_data = kmalloc(sizeof(struct device_data), GFP_KERNEL);

	if (!g_device_data) {
		exit_cleanup();
		return -ENOMEM;
	}

	// Initialize the device data with all 0s.
	memset(g_device_data, 0, sizeof(struct device_data));

	// Get file operations structure that will be used to initialize our device.
	struct file_operations * device_file_operations = get_file_operations();

	// Initialize this device buffer and memset it to 0.
    const int device_buffer_size = 100;
    g_device_data->m_device_buffer_size = device_buffer_size;
	g_device_data->m_device_buffer_data_len = 0;
    g_device_data->m_device_buffer = kmalloc(
        device_buffer_size * sizeof(char), GFP_KERNEL
    );

    if(!g_device_data->m_device_buffer) {
        exit_cleanup();
        return -ENOMEM;
    }

    memset(g_device_data->m_device_buffer, 0, device_buffer_size * sizeof(char));

    // Initialize mutex.
    mutex_init(&(g_device_data->m_mutex));

    // Initialize `cdev` kernel structure that represents this device.
    int initialize_device_result = initialize_device(
        &(g_device_data->m_cdev), device_file_operations
    );

    if(initialize_device_result < 0) {
        PRINT_DEBUG("initialize_device() has failed.\n");
    }

    // Create this device in sysfs, i.e. in `/dev/` directory.
    int create_sysfs_device_result = create_sysfs_device(g_module_name);

    if(create_sysfs_device_result) {
        PRINT_DEBUG("create_sysfs_device() has failed.\n");
    }

	return 0;
}

/**
 * @brief Exit function, which is called via `module_exit()`.
 *		  This function must be static.
 */
static void __exit my_driver_exit(void) {
	exit_cleanup();
	PRINT_DEBUG("__EXIT__ module %s\n", g_module_name);
}

module_init(my_driver_init);
module_exit(my_driver_exit);
