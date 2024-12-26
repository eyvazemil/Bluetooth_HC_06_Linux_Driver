/**
 * @brief File contains structure, which is used among different files,
 * that represents the data that each device holds.
 */

#ifndef DEVICE_DATA_H
#define DEVICE_DATA_H

/** Header that contains declaration of `cdev` structure. */
#include <linux/cdev.h>

/** Header that contains a mutex. */
#include <linux/mutex.h>

/** Header that contains completions. */
#include <linux/completion.h>

/**
 * Structure with the data for each device that we will allocate on heap.
 * For now it only has `cdev` structure that is associated with 
 * the given device and arbitrary device buffer size, 
 * but it could be populated with anything else we need.
 */
struct device_data {
    /**
     * Kernel structure of this device.
     */
	struct cdev m_cdev;

    /**
     * Mutex, which is locked and unlocked in `read()` and `write()` file 
     * operations to allow only one process to read from/write to this device.
     */
	struct mutex m_mutex;

    /**
     * Buffer, associated with this device, where data is read from or written to.
     */
    char * m_device_buffer;
	
	/**
     * Number of bytes allocated for the device buffer.
     */
	int m_device_buffer_size;

	/**
     * Number of characters written to the buffer (actual data size in the buffer).
     */
	int m_device_buffer_data_len;
};

#endif // DEVICE_DATA_H
