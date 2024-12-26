#include "char_device_registration.h"

/**
 * Major and minor numbers that will be assigned to our driver via 
 * `alloc_chrdev_region()` function.
 */
static dev_t g_major_minor_numbers = 0;

/**
 * Number of devices that we should register.
 */
static int g_num_devices = 1;

/**
 * The start range of the minor numbers of the devices
 * are considered to be 0.
 */
static const int MINOR_VERSION_RANGE_START = 0;

int register_driver_number(const char * module_name) {
    // Dynamically register major version and minor version range.
	// We do dynamic registration, so that the kernel gives us the 
    // major version, instead of doing it by ourselves.
	dev_t major_minor_numbers;

	int result = alloc_chrdev_region(&major_minor_numbers, 
		MINOR_VERSION_RANGE_START, 
		g_num_devices, module_name
	);

	g_major_minor_numbers = major_minor_numbers;

    return result;
}

void unregister_driver_number(void) {
    unregister_chrdev_region(g_major_minor_numbers, g_num_devices);
}

dev_t get_driver_number(void) {
    return g_major_minor_numbers;
}

/**
 * @brief Devices class that we will use to create 
 * individual device nodes in sysfs, i.e. in `/dev/`.
 */
static struct class * g_devices_class = NULL;

int create_device_class(const char * class_name) {
    g_devices_class = class_create(class_name);

    if(!g_devices_class) {
        return -1;
    }

    return 0;
}

void destroy_device_class(void) {
    class_unregister(g_devices_class);
    class_destroy(g_devices_class);
}

int initialize_device(struct cdev * device_cdev, 
    struct file_operations * device_file_operations
) {
    // Initialize the device.
    cdev_init(device_cdev, device_file_operations);

    // Although owner of `file_operations` structure is assigned to `cdev`
    // that we initilize in `cdev_init()`, we have to manually set the owner
    // of `cdev` structure, in case if the owner in `file_operations` hasn't 
    // been set.
    device_cdev->owner = THIS_MODULE;

    // Add the initialized `cdev` structure to the kernel.
    // After call to `cdev_add()`, our device is considered as 
    // "live" and our module must be ready.
    // We assign only one specific device per the device node we 
    // created in `/dev/`.
    // We have to create a device number for each device that we add,
    // as, although their major numbers will be the same, the minor
    // numbers are different.
    const dev_t DEVICE_NUMBER = MKDEV(MAJOR(g_major_minor_numbers), 
        MINOR(g_major_minor_numbers)
    );

    const int NUM_REAL_DEVICES_PER_DEVICE = 1;
    int result = cdev_add(device_cdev, DEVICE_NUMBER, 
        NUM_REAL_DEVICES_PER_DEVICE
    );

    return result;
}

void uninitialize_device(struct cdev * device_cdev) {
    cdev_del(device_cdev);
}

int create_sysfs_device(const char * device_name) {
    struct device * device = NULL;
    const dev_t DEVICE_NUMBER = MKDEV(MAJOR(g_major_minor_numbers), 
        MINOR(g_major_minor_numbers)
    );

    device = device_create(g_devices_class, NULL, DEVICE_NUMBER,
        NULL, "%s", device_name
    );

    if(!device) {
        return -1;
    }

    return 0;
}

void destroy_sysfs_device(void) {
    const dev_t DEVICE_NUMBER = MKDEV(MAJOR(g_major_minor_numbers), 
        MINOR(g_major_minor_numbers)
    );

    device_destroy(g_devices_class, DEVICE_NUMBER);
}
