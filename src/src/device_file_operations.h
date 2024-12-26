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
 * @brief Returns the `file_operations` structure that has implementation
 * of `open()`, `release()`, `read()`, and `write()`.
 */
struct file_operations * get_file_operations(void);

#endif // DEVICE_FILE_OPERATIONS_H
