/*
 * MIT License
 * 
 * Copyright (c) 2017 k4m1
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

MODULE_LICENSE("Dual MIT/GPL");

#define DRIVER_NAME "DVDD"
#define DEVICE "ebb"

typedef unsigned char byte;
typedef struct
{
	byte data[256];
	int size;
} device_io_data;

static int major;
static struct class *ebbchar_class_ptr;
static struct device *ebbchar_device_ptr;
static device_io_data *diode;    /* lol */

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct 
file_operations vuln_dev_driver_fops =
{
    .open       = device_open,
    .read       = device_read,
    .write      = device_write,
    .release    = device_release,
};

/* ===================== */
/* Device I/O operations */
/* ===================== */
static int 
device_open(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s Device %s opened\n", DRIVER_NAME, DEVICE);
	return 0;
}

/* VULNERABLE CODE STARTS ! */
static ssize_t 
device_read(struct file *file_ptr, char *buff,
		size_t len, loff_t *offset)
{
	int errc;

	errc = copy_to_user(buff, diode->data, diode->size);
	if (errc) {
		printk(KERN_ERR "%s Failed to send data!\n", DRIVER_NAME);
		return -EFAULT;
	}
	printk(KERN_INFO "%s %s copied to userland at %p\n",
			DRIVER_NAME,
			diode->data,
			buff);
	diode->size=0;
	memset(diode->data, 0xFF, 256);
	return 0;
}

static ssize_t 
device_write(struct file *file_ptr, const char *buff,
		size_t len, loff_t *offset)
{
	if (strlen(buff) != len) {
		return -EFAULT;
	}
	printk(KERN_INFO "%s User input passed sanity check.\n", DRIVER_NAME);
	sprintf(diode->data, buff, len);
	diode->size = len;
	return 0;
}

/* VULNERABLE CODE STOPS ! (I hope *cough*) */

static int 
device_release(struct inode *inode_ptr, struct file *file_ptr)
{
	printk(KERN_INFO "%s Device %s released\n", DRIVER_NAME, DEVICE);
	return 0;
}

/* ============================== */
/* initializon and exit functions */
/* ============================== */
static int 
DVDD_init(void)
{
	long err_ptr;
	printk(KERN_ALERT "Damn Vulnerable Device Driver loading...\n");
	
	diode = (device_io_data*) kmalloc(sizeof(device_io_data),
					GFP_KERNEL);

	if (!diode) {
		printk(KERN_ALERT "%s: Malloc failed!\n", DRIVER_NAME);
		return -EFAULT;
	}

	major = register_chrdev(0, DRIVER_NAME, &vuln_dev_driver_fops);
	if (major < 0) {
		printk(KERN_ALERT "Failed to get device major!\n");
		return major;
	}

	ebbchar_class_ptr = class_create(THIS_MODULE, DEVICE);
	if (IS_ERR(ebbchar_class_ptr)) {
		printk(KERN_ALERT "Failed to get device class!\n");
		unregister_chrdev(major, DRIVER_NAME);
		err_ptr = PTR_ERR(ebbchar_class_ptr);
		return err_ptr;
	}
	
	ebbchar_device_ptr = device_create(ebbchar_class_ptr,
					   0,
					   MKDEV(major, 0),
					   0,
					   DRIVER_NAME);
	if (IS_ERR(ebbchar_device_ptr)) {
		printk(KERN_ALERT "Failed to create device!\n");
		class_destroy(ebbchar_class_ptr);
		unregister_chrdev(major, DRIVER_NAME);
		err_ptr = PTR_ERR(ebbchar_device_ptr);
		return err_ptr;
	}
	printk(KERN_INFO "%s Loaded, %s device created\n", DRIVER_NAME, DEVICE);
	return 0;
}

static void 
DVDD_exit(void)
{
	device_destroy(ebbchar_class_ptr, MKDEV(major, 0));
  	class_unregister(ebbchar_class_ptr);
	class_destroy(ebbchar_class_ptr);
	unregister_chrdev(major, DRIVER_NAME);
	printk(KERN_INFO "%s has been unloaded!\n", DRIVER_NAME);
}

module_init(DVDD_init);
module_exit(DVDD_exit);

/* EOF */
