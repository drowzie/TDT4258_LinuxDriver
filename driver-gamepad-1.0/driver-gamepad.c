#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/signal.h>
#include "efm32gg.h"

//Define Interrupt number as given in the Compendium
#define GPIO_EVEN_INTERRUPT 17
#define GPIO_ODD_INTERRUPT 18

//Function Prototypes
static int btn_open (struct inode *, struct file *);
static int btn_release (struct inode *, struct file *);
static ssize_t btn_read (struct file *, char __user *, size_t, loff_t *);
static ssize_t btn_write (struct file *, const char __user *, size_t, loff_t *);
static void configure_GPIO (void);
static irqreturn_t gpioInterruptHandler(int, void *, struct pt_regs *);
static int async_response(int, struct file *, int);

//Custom Device Structure
static struct cdev custom_dev;
static char* device_name = "btn_gamepad";
static dev_t device_number;

//Memory Size from GPIO_PC_BASE up to GPIO_IF + 0x4 to include it
static u32 gpio_bytes = (u32)GPIO_IFC - (u32)GPIO_PC_BASE + 0x4;

//Asyncronous Queue
static struct fasync_struct *f_async_queue;

//Pointer for holding address returned from ioremap
static void * virtual_address;

//Device Class For Making Device Visble in User Space
static struct class *cl;

/*
 * Function for initializing kernel module
 * Sets up device number, cdev, memory and gpio
 * 
 * Returns 0 if successfull, otherwise -1 or -EBUSY
 */
static int __init btn_init(void)
{
	//Initialize structure for file operations
	static struct file_operations custom_fops = {
		.owner = THIS_MODULE,
		.read = btn_read,
		.write = btn_write,
		.open = btn_open,
		.release = btn_release,
		.fasync = async_response
	};

	//Initialize Driver By Getting Device Number
	if(alloc_chrdev_region(&device_number, 0, 1, device_name) != 0)
	{
		printk("Can't Allocate Device Number\n");
		return -1;
	}

	//Request Memory space for I/O from GPIO_PC_BASE as start
	if(request_mem_region(GPIO_PC_BASE, gpio_bytes, device_name) == NULL)
	{
		printk("Can't request Memory Region\n");
		return -EBUSY;
	}


	//Remap IO to Virtual Memory (Even if it's not going to be used)
	virtual_address = ioremap_nocache(GPIO_PC_BASE, gpio_bytes);


	//Initialize Custom Device
	cdev_init(&custom_dev, &custom_fops);

	//Create Class and Device
	cl = class_create(THIS_MODULE, device_name);
	device_create(cl, NULL, device_number, NULL, device_name);

	//Call configure_GPIO
	configure_GPIO();


	//Add the device to the system
	cdev_add(&custom_dev, device_number, 1);


	printk("Module is Loaded!\n");
	return 0;
}

/*
 * Function for cleaning up module in kernel space
 */

static void __exit btn_cleanup (void)
{
	//Free Used Interrupts
	free_irq((u32)GPIO_EVEN_INTERRUPT, &device_number);
	free_irq((u32)GPIO_ODD_INTERRUPT, &device_number);

	//Destory Device and Class
	device_destroy(cl, device_number);
	class_destroy(cl);
	
	//Release Memory
	release_mem_region(GPIO_PC_BASE, gpio_bytes);
	
	//Delete Custom Device from System
	cdev_del(&custom_dev);
	
	//Unregister Current Device Name
	unregister_chrdev_region(device_number, 1);
	printk("Module Unloaded\n");
}

/*
 * Function is called when User calls open on device 
 * Parameters: 
 * 		Inode pointer, struct inode *
 * 		File pointer, struct file *
 * Returns 0 on exit
 */

static int btn_open (struct inode *inode, struct file *filp)
{
  printk("Gamepad Driver Opened!\n");
  return 0;
}

/*
 * Function is called when User calls release on device
 * Parameters: 
 * 		Inode pointer, struct inode *
 * 		File pointer, struct file *
 * Returns 0 on exit
 */

static int btn_release (struct inode *inode, struct file *filp)
{
  printk("Gamepad Driver Released!\n");
  return 0;
}

/*
 * Function is called when User calls read on device
 * Parameters: 
 * 		File pointer, struct file *
 * 		Char buffer, const char __user *
 * 		Size_t count, size_t
 * 		Offset pointer, loff_t
 * 
 * Function reads the value of register GPIO_PC_DIN (Buttons)
 * and copies the value into the userspace
 * 
 * Returns 0
 */ 

static ssize_t btn_read (struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
   unsigned int readValue;
   readValue = ioread32(GPIO_PC_DIN);
   //Copy Data from the Kernel Space to The User Space
   //We can copy the whole thing (sizeof(readValue)) or just the first byte 0-7
   copy_to_user(buff, &readValue, 1);
   printk("Read from Gamepad Driver!\n");
   return 1;
}

/*
 * Function is called when User calls write on device
 * Parameters: 
 * 		File pointer, struct file *
 * 		Char buffer, const char __user *
 * 		Size_t count, size_t
 * 		Offset pointer, loff_t
 * 
 * Returns 0
 */ 

static ssize_t btn_write (struct file *filp , const char __user *buff , size_t count , loff_t *offp)
{	
	printk("Written to Gamepad Driver!\n");
	return 0;
}

/*
 * Function for configuring the GPIO
 */ 
static void configure_GPIO (void){
	//Setup Buttons (Input with Filter)
	iowrite32(0x33333333, GPIO_PC_MODEL);
	//Enable pull-up
	iowrite32(0xFF, GPIO_PC_DOUT);

	//Request Interrupt for GPIO ODD AND EVEN
	request_irq((u32)GPIO_EVEN_INTERRUPT, (irq_handler_t)gpioInterruptHandler, 0, device_name, &device_number);
	request_irq((u32)GPIO_ODD_INTERRUPT, (irq_handler_t)gpioInterruptHandler, 0, device_name, &device_number);
	//Enable Interrupt for GPIO
	iowrite32(0x22222222, GPIO_EXTIPSELL);
	iowrite32(0xFF, GPIO_EXTIFALL);
	iowrite32(0xFF, GPIO_IEN);
}

/*
 * Function for handeling GPIO interrupt
 * Parameters: 
 * 		Interrupt number, irq
 * 		Device Id pointer, *
 * 		Size_t count, count
 * Returns Interrupt return type, irqreturn_t
 */ 
static irqreturn_t gpioInterruptHandler(int irq, void *dev_id, struct pt_regs *regs)
{
	//Clear Interrupt Flag
	iowrite32(0xFFFF, GPIO_IFC);
	//Generate Signal to OS
	if(f_async_queue){
	kill_fasync(&f_async_queue, SIGIO, POLL_IN);
	}
	//Return that the Interrupt was handled
	return IRQ_HANDLED;
}

/*
 * Helper function for setting up asyncronous interrupt handling
 * Parameters:
 * 		File decription, int 
 * 		File pointer, struct file *
 * 		Mode, int
 * Returns result from fasync_helper, int
 */ 
static int async_response(int fd, struct file *filp, int mode){
	return fasync_helper(fd, filp, mode, &f_async_queue);
}

module_init(btn_init);
module_exit(btn_cleanup);

MODULE_DESCRIPTION("Gamepad module for TDT4258");
MODULE_LICENSE("GPL");


