/**
 * @brief File contains registration of driver's major and minor numbers
 * and connection of each device number with specific devices.
 */

#ifndef CHAR_DEVICE_REGISTRATION_H
#define CHAR_DEVICE_REGISTRATION_H

/**
 * Header that contains many definitions of functions and symbols
 * needed by the loadable modules.
 */
#include <linux/module.h>

/** Header that contains `dev_t` type for module major and minor numbers. */
#include <linux/types.h>

/** Header that contains `MAJOR()` and `MINOR()` macros for extraction of
 * major and minor numbers from `dev_t` variable.
 */
#include <linux/kdev_t.h>

/** Header that contains declaration of `cdev` structure.
 */
#include <linux/cdev.h>

/**
 * @brief Registers the major and minor numbers of our driver.
 * This function should be called at initilization of our module
 * and once it was called, `unregister_driver_number()` function
 * must be called at unitialization of our module.
 * Registration of the major number is dynamic, thus we don't have 
 * to supply our own major number.
 */

int register_driver_number(const char * module_name);
void unregister_driver_number(void);

/**
 * @brief Returns major and minor numbers of this driver.
 * Should be called after `register_driver_number()` and
 * before `unregister_driver_number()`.
 */
dev_t get_driver_number(void);

/**
 * @brief Creates the class of device, with which we can create multiple devices 
 * that will be managed by our driver. After calling this function, our devices
 * class can be found in `/sys/class` directory.
 *
 * @return Returns 0 on success and a non-zero number on failure.
 */
int create_device_class(const char * class_name);
void destroy_device_class(void);

/**
 * @brief Registers the given device in the kernel, i.e. initializes its 
 * `cdev` structure. Once this function is called, `cdev` structure is 
 * initialized and added to the kernel, thus this device will already be
 * "live" and its file operations functions, e.g. `open()`, `read()`, etc,
 * can already be called, thus our driver must be fully ready before calling
 * this function.
 * Once this function is called in the initialization function
 * of our module, don't forget to call `uninitialize_device()` function in
 * uninitialization function of our module.
 *
 * @param device_cdev `cdev` structure, which should already be allocated on heap.
 *
 * @return Returns 0 on success and non-zero value on failure.
 */
int initialize_device(struct cdev * device_cdev, 
    struct file_operations * device_file_operations
);

void uninitialize_device(struct cdev * device_cdev);

/**
 * @brief Creates an actual device in sysfs, i.e. created a device in `/dev/`.
 * Should be called for each device only if `create_devices_class()` has been
 * called. The device will be `/dev/<device_name>`.
 *
 * Once this function is called in the initilization function of our module,
 * don't forget to call `destroy_sysfs_device()` in uninitialization function
 * of our module.
 *
 * @return Returns 0 on success and non-zero value on failure.
 */
int create_sysfs_device(const char * device_name);
void destroy_sysfs_device(void);


#endif // CHAR_DEVICE_REGISTRATION_H
