#include "custom_macros.h"
#include "device_file_operations.h"
#include "device_data.h"

/**
 * Header that contains many definitions of functions and symbols
 * needed by the loadable modules.
 */
#include <linux/module.h>

/** Header with `file_operations` structure that we supply our functions
 * with in order for the user to be able to make system calls that will
 * call our functions.
 */
#include <linux/fs.h>

/** Header with declaration of `container_of()` function, which is used
 * to find the container, where a given member with the given value is 
 * located.
 */
#include <linux/kernel.h>

/** Header with declaration of `copy_to_user()` and `copy_from_user()`
 * functions.
 */
#include <asm/uaccess.h>

/** Header with declaration of error values. */
#include <linux/errno.h>


// -------------------------------------------------------------
// Declaration of `file_operations` structure and its functions.
// -------------------------------------------------------------

// We have to declare the prototypes of the functions first due to the
// `-Wmissing-prototypes` `gcc` flag that is supplied via kernel Makefile.
int device_open(struct inode * inode, struct file * filep);
int device_release(struct inode * inode, struct file * filep);

/**
 * @brief Reads `num_bytes` number of bytes from `user_buffer` 
 * starting from offset `file_offset` and increments its value 
 * by value the of `num_bytes`.
 *
 * @return Returns the number of bytes read from the user 
 * or `-EFAULT`, which means bad address, in case if the 
 * data couldn't be copied from device buffer to `user_buffer`.
 */
ssize_t device_read(
	struct file * filep, char __user * user_buffer,
	size_t num_bytes, loff_t * file_offset
);

ssize_t device_write(
	struct file * filep, const char __user * user_buffer,
	size_t num_bytes, loff_t * file_offset
);

struct file_operations g_file_operations = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write
};

struct file_operations * get_file_operations(void) {
    return &g_file_operations;
}

// -------------------------------------------------------
// Definition of functions in `file_operations` structure.
// -------------------------------------------------------

int device_open(struct inode * inode, struct file * filep) {
    // Retrieve the structure with our device data.
    // This is done via calling `container_of()` function, 
    // which finds the `m_cdev` member in `device_data` structure,
    // which is equal to the `cdev` structure in `inode`.
    struct device_data * device_data = container_of(
        inode->i_cdev, struct device_data, m_cdev
    );

    // Pass the found `device_data` structure as a private field
    // of `file` structure, which could be used in `read()` and 
    // `write()` functions.
    filep->private_data = device_data;

    printk(KERN_ALERT "device_open() was called on device.\n");

    return 0;
}

int device_release(struct inode * inode, struct file * filep) {
    struct device_data * device_data = filep->private_data;
    printk(KERN_ALERT "device_release() was called on device.\n");

    // There is not much to be done right now, as there is no actual 
    // hardware device that is connected to this device node, thus we
    // simply return.
    return 0;
}

ssize_t device_read(
	struct file * filep, char __user * user_buffer,
	size_t num_bytes, loff_t * file_offset
) {
    // Get the structure with device data from the `file` structure's
    // `private_data` member, that we set in `device_open()` function.
    struct device_data * device_data = filep->private_data;

    // As we are accessing the device data here, which could be written to by another process,
    // we have to lock on mutex before proceeding any further.
    // We lock in interruptible fashion, so that the user could kill the process. As this function,
    // once called, will be running in the user's process space and it wouldn't be killable by the user if this 
    // process will be waiting on a mutex, thus this waiting for mutex to be unlocked should be interruptible.
    // Function `mutex_lock_interruptible()` returns a non-zero code, once interrupted via user, thus we have to check
    // its return value and in case if it is non-zero, we return `-ERESTARTSYS`, which will make the kernel to
    // try to restart the call from the beginning or return an error to the user.
    if(mutex_lock_interruptible(&(device_data->m_mutex))) {
        // Waiting on mutex has been interrupted, thus no mutex was acquired and we don't have to unlock it.
        return -ERESTARTSYS;
    }

    // -- CRITICAL SECTION BEGIN --
    const int device_buffer_size = device_data->m_device_buffer_size;

    if(*file_offset >= device_buffer_size) {
        // If the file offset is already at the end of the device buffer
        // or is even beyond it, then we don't read anything from the device.
        // Before returning, we have to unlock the mutex.
        // -- CRITICAL SECTION END --
        mutex_unlock(&(device_data->m_mutex));
        return 0;
    }

    if(*file_offset + num_bytes >= device_buffer_size) {
        // Adjust the size of the buffer that we will read from the device,
        // if the size that we want to read will be going beyond the size
        // of the device buffer (taking into account the current file offset).
        num_bytes = device_buffer_size - *file_offset;
    }

    if(copy_to_user(user_buffer, ((char *) 
        device_data->m_device_buffer) + *file_offset, num_bytes)
    ) {
        // In case if copying to the user buffer has failed,
        // return `-EFAULT`, which means "bad address".
        // Before returning, we have to unlock the mutex.
        // -- CRITICAL SECTION END --
        mutex_unlock(&(device_data->m_mutex));
        return -EFAULT;
    }

    // Debug info.
    printk(KERN_ALERT "device_read()>> %zd bytes of data was read from device.\n", num_bytes);

    for(int i = 0; i < num_bytes; ++i) {
        printk(KERN_ALERT "%c", *(((char *) 
            device_data->m_device_buffer) + *file_offset + i)
        );
    }

    printk(KERN_ALERT "\n");

    // -- CRITICAL SECTION END --
    mutex_unlock(&(device_data->m_mutex));

    // Update the offset of the device buffer.
    *file_offset += num_bytes;

    // Return the number of bytes we read from the device.
    return num_bytes;
}

ssize_t device_write(
	struct file * filep, const char __user * user_buffer,
	size_t num_bytes, loff_t * file_offset
) {
    // Get the structure with device data from the `file` structure's
    // `private_data` member, that we set in `device_open()` function.
    struct device_data * device_data = filep->private_data;

    // The same logic with mutex locking as in `device_read()` function.
    if(mutex_lock_interruptible(&(device_data->m_mutex))) {
        // Waiting on mutex has been interrupted, thus no mutex was acquired and we don't have to unlock it.
        return -ERESTARTSYS;
    }

    // -- CRITICAL SECTION BEGIN --
    const int device_buffer_size = device_data->m_device_buffer_size;

    if(*file_offset >= device_buffer_size) {
        // If the file offset is already at the end of the device buffer
        // or is even beyond it, then we don't write anything to the device.
        // Before returning, we have to unlock the mutex.
        // -- CRITICAL SECTION END --
        mutex_unlock(&(device_data->m_mutex));
        return 0;
    }

    if(*file_offset + num_bytes >= device_buffer_size) {
        // Adjust the size of the buffer that we will write to the device,
        // if the size that we want to write will be going beyond the size
        // of the device buffer (taking into account the current file offset).
        num_bytes = device_buffer_size - *file_offset;
    }

    if(copy_from_user(((char *) device_data->m_device_buffer) + *file_offset,
        user_buffer, num_bytes)
    ) {
        // In case if copying to the user buffer has failed,
        // return `-EFAULT`, which means "bad address".
        // Before returning, we have to unlock the mutex.
        // -- CRITICAL SECTION END --
        mutex_unlock(&(device_data->m_mutex));
        return -EFAULT;
    }

    // Store the number of bytes that we copied from the user.
    device_data->m_device_buffer_data_len = num_bytes;

    // Debug info.
    printk(KERN_ALERT "device_write()>> %zd bytes of data was written to device.\n", num_bytes);

    for(int i = 0; i < num_bytes; ++i) {
        printk(KERN_ALERT "%c", *(((char *) 
            device_data->m_device_buffer) + *file_offset + i)
        );
    }

    printk(KERN_ALERT "\n");

    // -- CRITICAL SECTION END --
    mutex_unlock(&(device_data->m_mutex));

    // Update the offset of the device buffer.
    *file_offset += num_bytes;

    // Return the number of bytes we wrote to the device.
    return num_bytes;
}
