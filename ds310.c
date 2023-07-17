/**
 * Raspberry Pi driver for the DS310
 * 
 * This driver establishes I2C communication between the Raspberry Pi
 * and the DS310. It is used to read the temperature and humidity.
 * The DS310 Sensor can be configured over the device file. Interrupts
 * from the DS310 device are handled with a Interrupt Service Routine
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
#include <linux/i2c.h>

#define DRIVER_COMPATIBILITY "ds310_sensor"
#define DRIVER_NAME "ds310_sensor"
#define DRIVER_CLASS "ds310_sensor_class"
#define DS310_SENSOR_ADDRESS 0x77

static struct i2c_client *ds310_sensor_client;

/**
 * Variables for Character Device file
*/
struct dev_t ds310_sensor_device_number;
static struct class *ds310_sensor_class;
static struct cdev ds310_sensor_character_device;

static struct of_device_id ds310_sensor_of_match[] = {
    { .compatible = DRIVER_COMPATIBILITY, },
    { },
};
MODULE_DEVICE_TABLE(of, ds310_sensor_of_match);

/**
 * @brief This function is called, when the DS310 sensor device
 *       file is opened
*/
static int ds310_sensor_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "ds310_sensor_open\n");
    return 0;
}

/**
 * @brief This function is called, when the DS310 sensor device
 *       file is closed
*/
static int ds310_sensor_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "ds310_sensor_release\n");
    return 0;
}

/**
 * @brief Mapping file operations to the character device file
*/
static struct ds310_sensor_file_operations =
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
    if (client->addr != DS310_SENSOR_ADDRESS)
    {
        printk(KERN_ERR "ds310_sensor_probe: wrong device address 0x%x\n", client->addr);
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
    unregister_chrdev_region(ds310_sensor_device_number, DRIVER_NAME);
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
    unregister_chrdev_region(ds310_sensor_device_number, DRIVER_NAME);
}



