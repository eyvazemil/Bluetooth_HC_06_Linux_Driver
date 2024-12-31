#include "ftdi_usb_driver.h"
#include "custom_macros.h"
#include "device_file_operations.h"

#include <linux/sprintf.h>

#define FTDI_VENDOR_ID 0x0403
#define FTDI_PRODUCT_ID 0x6001
#define TIMER_SCHEDULE_JIFFIES 10

// -------------------------------------------------------------------------
// Definition of functions for allocating and freeing device data structure.
// -------------------------------------------------------------------------

/**
 * Device data for storing data from reading from USB bulk IN endpoint and 
 * writing to USB bulk OUT endpoint.
 */
static struct device_data * g_device_data = NULL;

/**
 * @brief Frees device data structure, thus it should be during device deregistration,
 * when we are sure that neither `read()` nor `write()` file operations can be called 
 * on this device.
 */
static void device_data_free(void) {
    if(g_device_data) {
		// Uninitialize this device only if the device data was successfully allocated.
		if(g_device_data->m_device_buffer) {
            // Unitialize this device if the device buffer was 
            // successfully allocated.
            kfree(g_device_data->m_device_buffer);
        }

		kfree(g_device_data);
	}
}

/**
 * @brief Allocates device data structure, which will be used in 
 * `read()` and `write()` file operations.
 * Should be called during device registration, before `read()` and `write()`
 * file operations could be called on this device.
 */
static int device_data_allocate(int usb_bulk_endpoint_max_packet_size) {
    // Allocate device data and memset it to 0.
	g_device_data = kmalloc(sizeof(struct device_data), GFP_KERNEL);

	if (!g_device_data) {
		device_data_free();
		return -ENOMEM;
	}

	memset(g_device_data, 0, sizeof(struct device_data));

	// Initialize this device buffer and memset it to 0. We set its value to the 
    // maximum packate size of USB bulk endpoint + 1 (for the ending NUL character).
    g_device_data->m_device_buffer_size = usb_bulk_endpoint_max_packet_size + 1;
	g_device_data->m_device_buffer_data_len = 0;
    g_device_data->m_device_buffer = kmalloc(
        usb_bulk_endpoint_max_packet_size * sizeof(char), GFP_KERNEL
    );

    if(!g_device_data->m_device_buffer) {
        device_data_free();
        return -ENOMEM;
    }

    memset(g_device_data->m_device_buffer, 0, usb_bulk_endpoint_max_packet_size * sizeof(char));

    // Initialize mutex.
    mutex_init(&(g_device_data->m_mutex));

    return 0;
}

// --------------------------------------------------------------------------------------------
// Definition of USB bulk IN/OUT endpoint operations along with timer to check those endpoints.
// --------------------------------------------------------------------------------------------

/**
 * Timer that is used for reading from the bulk IN endpoint.
 */
static struct timer_list timer_bulk_in;

/**
 * Timer that is used for writing to the bulk OUT endpoint.
 */
static struct timer_list timer_bulk_out;

/**
 * @brief Schedules the timer for provided jiffies value.
 */
static void schedule_timer(struct timer_list * timer, unsigned long timeout_jiffies) {
    const int is_timer_running = mod_timer(timer, timeout_jiffies);

	if(is_timer_running == 0) {
		// We will be getting this value, as the timer, at the point of its
		// handler being called is inactive, as its timeout has already happened.
		PRINT_DEBUG("schedule_timer(): timer was rescheduled.\n");
	} else {
		// We would get here, only if we are modifying the timer, which is still
		// active, i.e. its timeout hasn't happened and its handler hasn't been called yet,
		// which is not our case, as we are calling this from timer's handler.
		PRINT_DEBUG("schedule_timer(): timer is already running.\n");
	}
}

/**
 * @brief Called by timer to check USB bulk IN endpoint to make 
 * URB read transaction from USB device.
 */
static void timer_handler_bulk_in(struct timer_list * timer) {
    //retval = usb_bulk_msg(device, usb_rcvbulkpipe(device, BULK_EP_IN),
    //        bulk_buf, MAX_PKT_SIZE, &read_cnt, 5000);

    PRINT_DEBUG("timer_handler_bulk_in(): called.\n");

    // Reschedule this timer.
    schedule_timer(timer, TIMER_SCHEDULE_JIFFIES);
}

/**
 * @brief Called by timer to check USB bulk OUT endpoint to make 
 * URB write transaction to USB device.
 */
static void timer_handler_bulk_out(struct timer_list * timer) {
    PRINT_DEBUG("timer_handler_bulk_out(): called.\n");

    // Reschedule this timer.
    schedule_timer(timer, TIMER_SCHEDULE_JIFFIES);
}

// -------------------------------------
// Definition of `usb_driver` structure.
// -------------------------------------

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

int device_data_allocate(int usb_bulk_endpoint_max_packet_size);
void device_data_free(void);

/**
 * Module name and the class name, which will be used as USB device name and its class name
 * respectively.
 */
static char * g_usb_device_class_name = NULL;

int ftdi_usb_driver_register(char * usb_device_class_name, int usb_bulk_endpoint_max_packet_size) {
    g_usb_device_class_name = usb_device_class_name;

    // Allocate device data structure that will be used in `read()` and `write()` file operations.
    int device_data_error = device_data_allocate(usb_bulk_endpoint_max_packet_size);

    if(device_data_error) {
        PRINT_DEBUG("ftdi_usb_driver_register(): device data allocation failed with error code: %d\n", 
            device_data_error
        );

        return device_data_error;
    }

    // Create timers for bulk IN and OUT endpoints.
	const int flags = 0;
	timer_setup(&timer_bulk_in, &timer_handler_bulk_in, flags);
    timer_setup(&timer_bulk_out, &timer_handler_bulk_out, flags);

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

    // Delete timers. In order to make sure that one core doesn't destroy the timer, 
    // while another executes its handler, we have to use `del_timer_sync()` function,
	// instead of plain `del_timer()` function.
	const int is_timer_bulk_in_running = del_timer_sync(&timer_bulk_in);

	if(is_timer_bulk_in_running == 1) {
		PRINT_DEBUG("ftdi_usb_driver_deregister(): timer_bulk_in is still running after deleting.\n");
	} else {
		PRINT_DEBUG("ftdi_usb_driver_deregister(): timer_bulk_in was successfully deleted.\n");
	}

    const int is_timer_bulk_out_running = del_timer_sync(&timer_bulk_out);

	if(is_timer_bulk_out_running == 1) {
		PRINT_DEBUG("ftdi_usb_driver_deregister(): timer_bulk_out is still running after deleting.\n");
	} else {
		PRINT_DEBUG("ftdi_usb_driver_deregister(): timer_bulk_out was successfully deleted.\n");
	}

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
    g_usb_device_class.fops = get_file_operations(g_device_data);

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

    // Schedule both bulk IN and OUT timers.
    schedule_timer(&timer_bulk_in, TIMER_SCHEDULE_JIFFIES);
    schedule_timer(&timer_bulk_out, TIMER_SCHEDULE_JIFFIES);

    return 0;
}

static void driver_disconnect(struct usb_interface * interface) {
    usb_deregister_dev(interface, &g_usb_device_class);
}
