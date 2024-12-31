/**
 * @brief File contains the bulk of the USB driver code, specifically, driver registration,
 * deregistration, probing, and disconnecting.
 */

#ifndef FTDI_USB_DRIVER_H
#define FTDI_USB_DRIVER_H

#include <linux/usb.h>


/**
 * Registers our FTDI device USB driver.
 *
 * @param usb_device_class_name Will be used as a USB device class name.
 *
 * @return 0 on success, anything else on failure.
 */
int ftdi_usb_driver_register(char * usb_device_class_name, 
    int usb_bulk_endpoint_max_packet_size
);

/**
 * Registers our FTDI device USB driver.
 */
void ftdi_usb_driver_deregister(void);


#endif // FTDI_USB_DRIVER_H
