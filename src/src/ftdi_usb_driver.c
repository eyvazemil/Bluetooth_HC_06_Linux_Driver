#include "ftdi_usb_driver.h"
#include "custom_macros.h"
#include "device_file_operations.h"

#include <linux/sprintf.h>

#define FTDI_VENDOR_ID 0x0403
#define FTDI_PRODUCT_ID 0x6001

/**
 * Structure with our FTDI USB device product and device ids that our driver supports.
 * The vendor and product ids are passed to `USB_DEVICE` macro, which returns us the 
 * exact USB device. The vendor and product ids are obtained from the result of `lsusb` command.
 */
static struct usb_device_id g_ftdi_devices_table[] = {
    { USB_DEVICE(FTDI_VENDOR_ID, FTDI_PRODUCT_ID) },
    {}
};

static int driver_probe(struct usb_interface * interface, const struct usb_device_id * device_id);
static void driver_disconnect(struct usb_interface * interface);

/**
 * FTDI USB device driver structure with members:
 *  * `name`: name of this driver, which is also used as a fallback for probing 
 *      the devices this driver supports against this driver. Refer to comments of 
 *      `probe()` method  for more detail regarding device and driver matching.
 *
 *  * `probe()`: once a given device is plugged in, master USB driver on USB bus is searching
 *      for a hot-pluggable driver for that device and the way it does it is by searching for 
 *      this device's id, which is defined in `id_table` member of this structure in file 
 *      `/lib/modules/<kernel-version>/modules.alias` to see if this driver should be hot-plugged
 *      for this exact device. In case if matching doesn't occur based on `id_table` of this driver,
 *      matching by `name` is attempted.
 *      When USB bus master driver tries to match this plugged device with this driver, it calls a 
 *      `probe()` method of this driver and in case if this method return successfully, i.e. returns 0,
 *      this driver is matched agains that device. In case of failure, master driver looks for another
 *      driver from `modules.alias` file.
 *
 *  * `disconnect()`: gets called, when the device is diconnected.
 *  * `id_table()`: index of vendor and product ids of devices that this driver supports.
 */
 static struct usb_driver g_ftdi_usb_driver = {
    .name = "ftdi_usb_driver",
    .probe = driver_probe,
    .disconnect = driver_disconnect,
    .id_table = g_ftdi_devices_table,
};

/**
 * Module name and the class name, which will be used as USB device name and its class name
 * respectively.
 */
static char * g_usb_device_class_name = NULL;

int ftdi_usb_driver_register(char * usb_device_class_name) {
    g_usb_device_class_name = usb_device_class_name;

    // Allocate device data structure that will be used in `read()` and `write()` file operations.
    int device_data_error = device_data_allocate();

    if(device_data_error) {
        PRINT_DEBUG("ftdi_usb_driver_register(): device data allocation failed with error code: %d\n", 
            device_data_error
        );

        return device_data_error;
    }

    // Register this FTDI USB driver.
    const int usb_register_error = usb_register(&g_ftdi_usb_driver);

    if(usb_register_error) {
        PRINT_DEBUG("ftdi_usb_driver_register(): device registration failed with error code: %d\n", 
            usb_register_error
        );
    } else{
        PRINT_DEBUG("ftdi_usb_driver_register(): device was successfully registered.\n");
    }

    return usb_register_error;
}

void ftdi_usb_driver_deregister(void) {
    // Deregister this FTDI USB driver.
    usb_deregister(&g_ftdi_usb_driver);

    // Free the device data structure, allocated for this device.
    device_data_free();

    PRINT_DEBUG("ftdi_usb_driver_deregister(): device was deregestered.\n");
}

/**
 * Structures with our usb device and its class, which will be initialized in `probe()` 
 * method of our driver.
 */
static struct usb_device * g_usb_device = NULL;
static struct usb_class_driver g_usb_device_class;

static int driver_probe(struct usb_interface * interface, const struct usb_device_id * device_id) {
    // Get USB device from its interface.
    g_usb_device = interface_to_usbdev(interface);

    // Instantiate USB device class with its name and file operations.
    // For that, we have to create a class name string like so: `usb/<usb_device_class_name>%d`,
    // where `%d` will be filled via USB core with the minor number of the device.
    // We have to create this string by allocating a new one that has `usb/` string + 4 for `%d`
    // (we reserve up to 3-digit minor number, just in case, along with terminating NUL character) + 
    // the length of our usb device class name, i.e. the custom name that we want our device to have 
    // in `/dev/` directory along with its minor number.
    const char * str_usb = "usb/";
    const char * str_minor_number_placeholder = "%d";
    const int str_alloc_size = strlen(g_usb_device_class_name) + strlen(str_usb) + 4;
    char * new_usb_class_name_str = kmalloc(str_alloc_size * sizeof(char), GFP_KERNEL);

    snprintf(new_usb_class_name_str, str_alloc_size * sizeof(char), "%s%s%s",
        str_usb, g_usb_device_class_name, str_minor_number_placeholder
    );

    new_usb_class_name_str[strlen(new_usb_class_name_str)] = '\0';

    g_usb_device_class.name = new_usb_class_name_str;
    g_usb_device_class.fops = get_file_operations();

    // Now register the USB device, so that the kernel creates it a file in sysfs, 
    // i.e. in `/dev/` directory.
    int registration_status = usb_register_dev(interface, &g_usb_device_class);

    if (registration_status) {
        PRINT_DEBUG("driver_probe(): couldn't register a USB device with status: %d.\n",
            registration_status
        );
    } else {
        PRINT_DEBUG("driver_probe(): successfully registered a USB device with minor number: %d\n",
            interface->minor
        );
    }

    // Once registration of USB device is done, we can free the string that we allocated for its name.
    kfree(new_usb_class_name_str);

    return registration_status;
}

static void driver_disconnect(struct usb_interface * interface) {
    usb_deregister_dev(interface, &g_usb_device_class);
}
