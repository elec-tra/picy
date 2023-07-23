/**
 * Raspberry Pi driver for the ds310 sensor
 * 
 * This driver establishes I2C communication between the Raspberry Pi
 * and the ds310. It is used to read the temperature and humidity.
 * The ds310 Sensor can be configured over the device file. Interrupts
 * from the ds310 device are handled with a Interrupt Service Routine
 * (ISR). In the ISR, the temperature and humidity are read and stored
 * in a shared memory space between kernel and user space. The Linux
 * kernel module send signals to the user space application when shared
 * memory reached a certain threshold. Then, the user space application
 * unload the shared memory.
 * 
 * Note: This module needs device tree overlay to be loaded. The device
 * tree overlay is also part of this project.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/i2c.h>

#define VERSION "1.0"
#define DRIVER_COMPATIBILITY "infineon,ds310_sensor"
#define DRIVER_NAME "ds310_sensor"
#define DRIVER_CLASS "ds310_sensor_class"
#define DS310_SENSOR_ADDRESS 0x77

static struct i2c_client *ds310_sensor_client;

/**
 * Variables for Character Device file
 */
static dev_t ds310_sensor_device_number;
static struct class *ds310_sensor_class;
static struct cdev ds310_sensor_character_device;
static uint8_t register_value = 0x00;

static struct of_device_id ds310_sensor_of_match[] = {
    { .compatible = DRIVER_COMPATIBILITY, },
    { },
};
MODULE_DEVICE_TABLE(of, ds310_sensor_of_match);

static struct i2c_device_id ds310_sensor_id[] = {
    { DRIVER_NAME, 0 },
    { },
};
MODULE_DEVICE_TABLE(i2c, ds310_sensor_id);

/**
 * @brief This function is called, when the ds310 sensor device
 *       file is opened
 */
static int ds310_sensor_open(struct inode *inode, struct file *device_file)
{
    printk(KERN_INFO "ds310_sensor_open\n");
    return 0;
}

/**
 * @brief This function is called, when the ds310 sensor device
 *       file is closed
 */
static int ds310_sensor_release(struct inode *inode, struct file *device_file)
{
    printk(KERN_INFO "ds310_sensor_release\n");
    return 0;
}

/**
 * @brief Send ds310 sensor register value to the user space
 */
static ssize_t ds310_sensor_read(struct file *device_file, char __user *user_buffer, size_t length, loff_t *offset)
{
    printk(KERN_INFO "ds310_sensor_read\n");

    uint8_t to_copy = 0, not_copied = 0, delta = 0;

    /* Decide amount of bytes to copy */
    to_copy = min(length, sizeof(register_value));

    /* Copy register value to user space */
    not_copied = copy_to_user(user_buffer, &register_value, to_copy);

    /* Calculate delta */
    delta = to_copy - not_copied;

    return delta;
}

/**
 * @brief Receive ds310 sensor register address or value or both
 */
static ssize_t ds310_sensor_write(struct file *device_file, const char __user *user_buffer, size_t length, loff_t *offset)
{
    printk(KERN_INFO "ds310_sensor_write\n");

    uint8_t to_copy = 0, not_copied = 0, delta = 0;
    uint8_t buffer[2] = {0};

    /* Decide amount of bytes to copy */
    to_copy = min(length, sizeof(buffer));

    /* Copy register address or value or both to kernel space */
    not_copied = copy_from_user(buffer, user_buffer, to_copy);

    if(length == 1)
    {
        /* Read register value */
        register_value = i2c_smbus_read_byte_data(ds310_sensor_client, buffer[0]);
    }
    else if(length == 2)
    {
        /* Write register value */
        i2c_smbus_write_byte_data(ds310_sensor_client, buffer[0], buffer[1]);
    }
    else
    {
        printk(KERN_ERR "ds310_sensor_write: wrong length\n");
    }

    /* Calculate delta */
    delta = to_copy - not_copied;

    return delta;
}

/**
 * @brief Mapping file operations to the character device file
 */
static struct file_operations ds310_sensor_file_operations =
{
    .owner = THIS_MODULE,
    .open = ds310_sensor_open,
    .release = ds310_sensor_release,
    .read = ds310_sensor_read,
    .write = ds310_sensor_write,
};

/**
 * @brief This function is called during loading the driver
 */
static int ds310_sensor_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk(KERN_INFO "ds310_sensor_probe\n");

    /**
     * Check if the device is ds310 pressure and temperature sensor
    */
    if ((client->addr != DS310_SENSOR_ADDRESS) || (id->name != DRIVER_NAME))
    {
        printk(KERN_ERR "ds310_sensor_probe: wrong device %s of address 0x%x\n", id->name, client->addr);
        return -ENODEV;
    }

    ds310_sensor_client = client;

    /**
     * Creating device file for ds310 sensor
     */
    /* Allocate Device Number */
    if (alloc_chrdev_region(&ds310_sensor_device_number, 0, 1, DRIVER_NAME) < 0)
    {
        printk(KERN_ERR "ds310_sensor_probe: alloc_chrdev_region failed\n");
        goto DEVICE_NUMBER_ERROR;
    }

    /* Create Device Class */
    if ((ds310_sensor_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL)
    {
        printk(KERN_ERR "ds310_sensor_probe: class_create failed\n");
        goto DEVICE_CLASS_ERROR;
    }

    /* Create Character Device file */
    if (device_create(ds310_sensor_class, NULL, ds310_sensor_device_number, NULL, DRIVER_NAME) == NULL)
    {
        printk(KERN_ERR "ds310_sensor_probe: device_create failed\n");
        goto DEVICE_FILE_ERROR;
    }

    /* Initialize Character Device file */
    cdev_init(&ds310_sensor_character_device, &ds310_sensor_file_operations);

    /* Add Character Device file to the system */
    if (cdev_add(&ds310_sensor_character_device, ds310_sensor_device_number, 1) == -1)
    {
        printk(KERN_ERR "ds310_sensor_probe: cdev_add failed\n");
        goto KERNEL_ERROR;
    }

    return 0;

KERNEL_ERROR:
    device_destroy(ds310_sensor_class, ds310_sensor_device_number);
DEVICE_FILE_ERROR:
    class_destroy(ds310_sensor_class);
DEVICE_CLASS_ERROR:
    unregister_chrdev(ds310_sensor_device_number, DRIVER_NAME);
DEVICE_NUMBER_ERROR:
    return -1;
}

/**
 * @brief This function is called during unloading the driver
 */
static void ds310_sensor_remove(struct i2c_client *client)
{
    printk(KERN_INFO "ds310_sensor_remove\n");

    /**
     * Remove device file for ds310 sensor
     */
    cdev_del(&ds310_sensor_character_device);
    device_destroy(ds310_sensor_class, ds310_sensor_device_number);
    class_destroy(ds310_sensor_class);
    unregister_chrdev(ds310_sensor_device_number, DRIVER_NAME);
}

/**
 * @brief I2C driver structure
 */
static struct i2c_driver ds310_sensor_driver =
{
    .driver =
    {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = ds310_sensor_of_match,
    },
    .probe = ds310_sensor_probe,
    .remove = ds310_sensor_remove,
    .id_table = ds310_sensor_id
};

module_i2c_driver(ds310_sensor_driver);

MODULE_AUTHOR("elec-tra");
MODULE_DESCRIPTION("Raspberry Pi driver for the ds310 sensor");
MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION);