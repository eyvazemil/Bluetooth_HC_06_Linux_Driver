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

/**
 * @brief Allocates device data structure, which will be used in 
 * `read()` and `write()` file operations.
 * Should be called during device registration, before `read()` and `write()`
 * file operations could be called on this device.
 */
int device_data_allocate(void);

/**
 * @brief Frees device data structure, thus it should be during device deregistration,
 * when we are sure that neither `read()` nor `write()` file operations can be called 
 * on this device.
 */
void device_data_free(void);

/**
 * @brief Returns the `file_operations` structure that has implementation
 * of `open()`, `release()`, `read()`, and `write()`.
 */
struct file_operations * get_file_operations(void);

#endif // DEVICE_FILE_OPERATIONS_H
