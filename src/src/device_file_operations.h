/**
 * @brief File contains file operations with our custom functions 
 * supplied, that will be called with corresponding system call.
 * Example: if `open()` was called on the device that is 
 * managed by this driver, then `file_operations.open()` 
 * will be called, which will call our own implementation
 * of `open()`.
 */

#ifndef DEVICE_FILE_OPERATIONS_H
#define DEVICE_FILE_OPERATIONS_H

#include "device_data.h"

/**
 * @brief Returns the `file_operations` structure that has implementation
 * of `open()`, `release()`, `read()`, and `write()`.
 *
 * @param device_data Structure that will be used to read from/write to the device file in sysfs.
 */
struct file_operations * get_file_operations(struct device_data * device_data);

#endif // DEVICE_FILE_OPERATIONS_H
