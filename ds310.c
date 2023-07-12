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