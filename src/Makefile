KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
BUILD_DIR = Build
SRC_DIR = src

# Will be used as the name of our kernel module.
MODULE_NAME = emil_bluetooth_driver
KERNEL_OBJECT_NAME = $(MODULE_NAME).ko
CHMOD_PERM = 666

# Will be used as the name of our USB device in sysfs, i.e. `/dev/` directory.
DEVICE_CLASS_NAME = emil_hc_06_dev

# Major version of the driver, which is read from the `/proc/devices`
# file, once `insmod` command has been called with the driver `.ko` file 
# and the driver has already registered itself via `alloc_chrdev_region()` 
# function in the driver's C code. 
MAJOR_VERSION=$$(awk "\$$2==\"${MODULE_NAME}\" {print \$$1}" /proc/devices)

# Object files to be built.
# Here we put the name of the module, which is represented as an
# object file, i.e. if our module is located in the C file <module-name>.c,
# the module will be represented as <module-name>.o .
obj-m += $(MODULE_NAME).o

# In case if `hello.c` includes other files, e.g. `file1.c` and `file2.c`,
# we can list them, as it is shown below (via <module_name>-objs).
emil_bluetooth_driver-objs += $(SRC_DIR)/main.o $(SRC_DIR)/device_file_operations.o \
	$(SRC_DIR)/ftdi_usb_driver.o

# We set the macro `DEBUG_MODE` in our code, to indicate that we are executing
# in debug mode, thus we can print messages for debugging.
ccflags-y += -std=gnu99 -Wno-declaration-after-statement -Wno-unused-function -Wno-unused-label -Wno-unused-variable -DDEBUG_MODE

# Build the `char_driver.o` object file, along with `char_driver.ko`, which will be an
# actual kernel object file, that we will supply to `insmod` to initialize the module.
all:
	@if [ ! -e $(BUILD_DIR) ]; then \
		mkdir ${BUILD_DIR}; \
	fi \

	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	mv $(SRC_DIR)/*.o $(SRC_DIR)/.*.cmd *.o .*.cmd *.ko *.mod.c *.mod modules* .module-common* Module* ${BUILD_DIR}

# Loads the module via calling `insmod` on the built kernel object, i.e. `.ko` object.
# Also creates a file per device in `/dev/` directory via `mknod` command. Before 
# creating a file, we remove the old file associated with this device.
# We first have to call `insmod` with our driver's object file, where driver 
# registers itself via `alloc_chrdev_region()` function, then we create device
# files associated with this driver via `mknod` command. `mknod` command requires
# a major number of the driver, which can be read with `awk` command from 
# `/proc/devices` file, where our driver and its major version are written to.
load:
	sudo insmod $(BUILD_DIR)/$(KERNEL_OBJECT_NAME) g_module_name="${MODULE_NAME}" \
		g_device_class_name="${DEVICE_CLASS_NAME}"

# 	Set permissions to the created device in sysfs.
	sudo chmod 666 /dev/${DEVICE_CLASS_NAME}0

# Prints `Error 1` if our module hasn't been loaded, otherwise,
# it returns our module name.
find_module:
	lsmod | grep $(MODULE_NAME)

# Systemd keeps journal that has kernel logs, which could be printed 
# to the screen via `journalctl`.
check_logs:
	sudo dmesg | tail -10

# Unload our module via `rmmod` command. Device files associtated with this 
# driver in `/dev/` directory will be removed in the exit function.
unload:
	sudo rmmod $(BUILD_DIR)/$(KERNEL_OBJECT_NAME)

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -rf $(BUILD_DIR)
