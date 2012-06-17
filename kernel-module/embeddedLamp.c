	/* embeddedLamp.c
   A driver for multiple LEDs over SPI and GPIO for displaying various information
   on a Pandaboard.

   2012 (C) Bartholomaeus Dedersen, Fachhochschule Kiel, Germany

   Loosely based on works of Scott Ellis
   https://github.com/scottellis/embeddedLamp/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

*/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/spi/spi.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/delay.h>

// the PIN used for latching
#define GPIO_PIN 140


#define NANOSECS_PER_SEC 1000000000
#define SPI_BUFF_SIZE	40
#define USER_BUFF_SIZE	128

// SPI configuration, every SPI transfer consists of 5 8bit messages
#define SPI_BUS 1
#define SPI_BUS_CS1 1 			// chip select
///#define SPI_BUS_SPEED 1000000 	// 10 MHz
#define SPI_BUS_SPEED 50000 	// 10 MHz

#define DEFAULT_WRITE_FREQUENCY 10		// in Hz, frequency in which messages are being sent
static int write_frequency = DEFAULT_WRITE_FREQUENCY;
module_param(write_frequency, int, S_IRUGO);
MODULE_PARM_DESC(write_frequency, "Spike write frequency in Hz");


const char this_driver_name[] = "embeddedLamp";

// SPI control message
struct embeddedLamp_control {
	struct spi_message msg;
	struct spi_transfer transfer;
	u32 busy;
	u32 spi_callbacks;
	u32 busy_counter;
	u8 *tx_buff; 
};

static struct embeddedLamp_control embeddedLamp_ctl;

// device driver configuration for SPI
struct embeddedLamp_dev {
	spinlock_t spi_lock;
	struct semaphore fop_sem;
	dev_t devt;
	struct cdev cdev;
	struct class *class;
	struct spi_device *spi_device;
	struct hrtimer timer;
	u32 timer_period_sec;
	u32 timer_period_ns;
	u32 running;
	char *user_buff;
};

static struct embeddedLamp_dev embeddedLamp_dev;



// callback, after each completed message transmission (consists of 5 8bit messages)
static void embeddedLamp_completion_handler(void *arg)
{	
	// try 1ms delay
	//mdelay(1);
	//set latching for LED over GPIO
	gpio_set_value(GPIO_PIN,1);
	//wait 5ms
	//mdelay(5);
	embeddedLamp_ctl.spi_callbacks++;
	embeddedLamp_ctl.busy = 0;
    	gpio_set_value(GPIO_PIN,0);
}



// used to write the messages into the FIFO queue (queue is being handled by kernel)
static int embeddedLamp_queue_spi_write(u8 *msg)
{
	int status;
	unsigned long flags;
	u8 tempNumber;
	int i;


	spi_message_init(&embeddedLamp_ctl.msg);

	/* this gets called when the spi_message completes */
	embeddedLamp_ctl.msg.complete = embeddedLamp_completion_handler;
	embeddedLamp_ctl.msg.context = NULL;

	/* write message from command line */
	for(i = 0; i < 5; i++)
		embeddedLamp_ctl.tx_buff[i] = msg[i];

	// transfer parameters
	embeddedLamp_ctl.transfer.tx_buf = embeddedLamp_ctl.tx_buff;	// send buffer
	embeddedLamp_ctl.transfer.rx_buf = NULL;					// we dont need the receive buffer
	embeddedLamp_ctl.transfer.len = 5;							// number of messages

	// add messages to SPI transfer
	spi_message_add_tail(&embeddedLamp_ctl.transfer, &embeddedLamp_ctl.msg);

	// lock interrupt service routine
	spin_lock_irqsave(&embeddedLamp_dev.spi_lock, flags);

	if (embeddedLamp_dev.spi_device)
		status = spi_async(embeddedLamp_dev.spi_device, &embeddedLamp_ctl.msg);	// transmit the transfer
	else
		status = -ENODEV;

	spin_unlock_irqrestore(&embeddedLamp_dev.spi_lock, flags);
	
	// if the transfer could not be sent, remember that it didn't work
	if (status == 0)
		embeddedLamp_ctl.busy = 1;
	
	return status;	
}



/**
 * 
 */
static enum hrtimer_restart embeddedLamp_timer_callback(struct hrtimer *timer)
{
	static u8 msg[] = {0,0,0,0,0};
	static int countArrayPart = 0;

	if (!embeddedLamp_dev.running) {
		return HRTIMER_NORESTART;
	}

	/* busy means the previous message has not completed */
	if (embeddedLamp_ctl.busy) {
		embeddedLamp_ctl.busy_counter++;
	}
	else if (embeddedLamp_queue_spi_write(msg) != 0) {
		return HRTIMER_NORESTART;
	}

	hrtimer_forward_now(&embeddedLamp_dev.timer, 
		ktime_set(embeddedLamp_dev.timer_period_sec, 
			embeddedLamp_dev.timer_period_ns));

	/* increase a running counter for demo display */
	// did we reach the maximum value?
	if(msg[countArrayPart] == 0xFF){
		if(countArrayPart < 5)
			countArrayPart++;
		else{
			// reset the 5 byte
			countArrayPart = 0;
			memset(msg,0,10);
		}	
	}else{  // we did not reach the maximum value, yet
		msg[countArrayPart] <<= 1;
		msg[countArrayPart] |= 0x1;
	}
	
	// restart the timer
	return HRTIMER_RESTART;
}



/**
 * Used to read statistics from the character device /dev/embeddedLamp
 */
static ssize_t embeddedLamp_read(struct file *filp, char __user *buff, size_t count,
			loff_t *offp)
{
	size_t len;
	ssize_t status = 0;

	if (!buff) 
		return -EFAULT;

	if (*offp > 0) 
		return 0;

	if (down_interruptible(&embeddedLamp_dev.fop_sem)) 
		return -ERESTARTSYS;

	sprintf(embeddedLamp_dev.user_buff, 
			"%s|%u|%u\n",
			embeddedLamp_dev.running ? "Running" : "Stopped",
			embeddedLamp_ctl.spi_callbacks,
			embeddedLamp_ctl.busy_counter);
		
	len = strlen(embeddedLamp_dev.user_buff);
 
	if (len < count) 
		count = len;

	if (copy_to_user(buff, embeddedLamp_dev.user_buff, count))  {
		printk(KERN_ALERT "embeddedLamp_read(): copy_to_user() failed\n");
		status = -EFAULT;
	} else {
		*offp += count;
		status = count;
	}

	up(&embeddedLamp_dev.fop_sem);

	return status;	
}



/**
 * We accept two commands 'start' or 'stop' and ignore anything else.
 * This function writes to the character device
 */
static ssize_t embeddedLamp_write(struct file *filp, const char __user *buff,
		size_t count, loff_t *f_pos)
{
	size_t len;	
	ssize_t status = 0;

	if (down_interruptible(&embeddedLamp_dev.fop_sem))
		return -ERESTARTSYS;

	memset(embeddedLamp_dev.user_buff, 0, 16);
	len = count > 16 ? 16 : count;

	if (copy_from_user(embeddedLamp_dev.user_buff, buff, len)) {
		status = -EFAULT;
		goto embeddedLamp_write_done;
	}

	/* we'll act as if we looked at all the data */
	status = count;

        printk(KERN_ALERT "embeddedLamp_dev.user_buff: %s\n", embeddedLamp_dev.user_buff);
       	printk(KERN_ALERT "len(embeddedLamp_dev.user_buff): %i\n", strlen(embeddedLamp_dev.user_buff));

	/* but we only care about the first 5 characters */
	if (!strnicmp(embeddedLamp_dev.user_buff, "start", 5)) {
		if (embeddedLamp_dev.running) {
			printk(KERN_ALERT "already running\n");
			goto embeddedLamp_write_done;
		}

		if (embeddedLamp_ctl.busy) {
			printk(KERN_ALERT "waiting on a spi transaction\n");
			goto embeddedLamp_write_done;
		}

		embeddedLamp_ctl.spi_callbacks = 0;		
		embeddedLamp_ctl.busy_counter = 0;

		hrtimer_start(&embeddedLamp_dev.timer, 
				ktime_set(embeddedLamp_dev.timer_period_sec, 
					embeddedLamp_dev.timer_period_ns),
        	               	HRTIMER_MODE_REL);

		embeddedLamp_dev.running = 1; 
	} 
	// should we stop here?
	else if (!strnicmp(embeddedLamp_dev.user_buff, "stop", 4)) {
		hrtimer_cancel(&embeddedLamp_dev.timer);
		embeddedLamp_dev.running = 0;
	}
	else if (strlen(embeddedLamp_dev.user_buff) == 11) {
        	u8 msg[] = {0,0,0,0,0};
		char transString[2];	
		int i;
		for(i = 0; i < 10; i = i + 2) {
			transString[0] = embeddedLamp_dev.user_buff[i];
			//printk(KERN_ALERT "transstring[0]: %s\n", transString[0]);
			transString[1] = embeddedLamp_dev.user_buff[i+1];
			//printk(KERN_ALERT "transstring[1]: %s\n", transString[1]);
			msg[i/2] = simple_strtol(transString,NULL,16);
			//printk(KERN_ALERT "msg[i/2]: %x", msg[i/2]);
                }
		embeddedLamp_queue_spi_write(msg);
		embeddedLamp_dev.running = 1; 
        }

embeddedLamp_write_done:

	up(&embeddedLamp_dev.fop_sem);

	return status;
}



/**
 * Creates /dev/embeddedLamp in the file system
 */
static int embeddedLamp_open(struct inode *inode, struct file *filp)
{	
	int status = 0;

	if (down_interruptible(&embeddedLamp_dev.fop_sem)) 
		return -ERESTARTSYS;

	if (!embeddedLamp_dev.user_buff) {
		embeddedLamp_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);
		if (!embeddedLamp_dev.user_buff) 
			status = -ENOMEM;
	}	

	up(&embeddedLamp_dev.fop_sem);

	return status;
}



/**
 * (Dummy function) Callback to enumerate SPI slaves
 */
static int embeddedLamp_probe(struct spi_device *spi_device)
{
	unsigned long flags;

	spin_lock_irqsave(&embeddedLamp_dev.spi_lock, flags);
	embeddedLamp_dev.spi_device = spi_device;
	spin_unlock_irqrestore(&embeddedLamp_dev.spi_lock, flags);

	return 0;
}



/**
 * Remove device /dev/embeddedLamp from file system
 */
static int embeddedLamp_remove(struct spi_device *spi_device)
{
	unsigned long flags;

	if (embeddedLamp_dev.running) {
		hrtimer_cancel(&embeddedLamp_dev.timer);
		embeddedLamp_dev.running = 0;
	}

	spin_lock_irqsave(&embeddedLamp_dev.spi_lock, flags);
	embeddedLamp_dev.spi_device = NULL;
	spin_unlock_irqrestore(&embeddedLamp_dev.spi_lock, flags);

	return 0;
}



/**
 * Used to connect to SPI bus
 */
static int __init add_embeddedLamp_device_to_bus(void)
{
	struct spi_master *spi_master;
	struct spi_device *spi_device;
	struct device *pdev;
	char buff[64];
	int status = 0;

	spi_master = spi_busnum_to_master(SPI_BUS);
	if (!spi_master) {
		printk(KERN_ALERT "spi_busnum_to_master(%d) returned NULL\n",
			SPI_BUS);
		printk(KERN_ALERT "Missing modprobe omap2_mcspi?\n");
		return -1;
	}

	spi_device = spi_alloc_device(spi_master);
	if (!spi_device) {
		put_device(&spi_master->dev);
		printk(KERN_ALERT "spi_alloc_device() failed\n");
		return -1;
	}

	spi_device->chip_select = SPI_BUS_CS1;

	/* Check whether this SPI bus.cs is already claimed */
	snprintf(buff, sizeof(buff), "%s.%u", 
			dev_name(&spi_device->master->dev),
			spi_device->chip_select);

	pdev = bus_find_device_by_name(spi_device->dev.bus, NULL, buff);
 	if (pdev) {
		/* We are not going to use this spi_device, so free it */ 
		spi_dev_put(spi_device);
		
		/* 
		 * There is already a device configured for this bus.cs  
		 * It is okay if it us, otherwise complain and fail.
		 */
		if (pdev->driver && pdev->driver->name && 
				strcmp(this_driver_name, pdev->driver->name)) {
			printk(KERN_ALERT 
				"Driver [%s] already registered for %s\n",
				pdev->driver->name, buff);
			status = -1;
		} 
	} else {
		spi_device->max_speed_hz = SPI_BUS_SPEED;
		spi_device->mode = SPI_MODE_0;
		spi_device->bits_per_word = 8;
		spi_device->irq = -1;
		spi_device->controller_state = NULL;
		spi_device->controller_data = NULL;
		strlcpy(spi_device->modalias, this_driver_name, SPI_NAME_SIZE);
		
		status = spi_add_device(spi_device);		
		if (status < 0) {	
			spi_dev_put(spi_device);
			printk(KERN_ALERT "spi_add_device() failed: %d\n", 
				status);		
		}				
	}

	put_device(&spi_master->dev);

	return status;
}



/**
 * This struct is used to set SPI parameters in the kernel
 */
static struct spi_driver embeddedLamp_driver = {
	.driver = {
		.name =	this_driver_name,
		.owner = THIS_MODULE,
	},
	.probe = embeddedLamp_probe,
	.remove = __devexit_p(embeddedLamp_remove),	
};


/**
 * Initialize SPI
 */
static int __init embeddedLamp_init_spi(void)
{
	int error;


	//omap_mux_init_gpio(GPIO_PIN, OMAP_PIN_INPUT);
	error = gpio_request(GPIO_PIN, "Latch-Bit");
        if (error) {
            printk(KERN_ALERT "failed to request GPIO %d: %d\n", GPIO_PIN, error);
            goto embeddedLamp_init_error;
        }
            
        gpio_direction_output(GPIO_PIN, 0);



	embeddedLamp_ctl.tx_buff = kmalloc(SPI_BUFF_SIZE, GFP_KERNEL | GFP_DMA);
	if (!embeddedLamp_ctl.tx_buff) {
		error = -ENOMEM;
		goto embeddedLamp_init_error;
	}

	error = spi_register_driver(&embeddedLamp_driver);
	if (error < 0) {
		printk(KERN_ALERT "spi_register_driver() failed %d\n", error);
		goto embeddedLamp_init_error;
	}

	error = add_embeddedLamp_device_to_bus();
	if (error < 0) {
		printk(KERN_ALERT "add_embeddedLamp_to_bus() failed\n");
		spi_unregister_driver(&embeddedLamp_driver);
		goto embeddedLamp_init_error;	
	}

	return 0;

embeddedLamp_init_error:

	if (embeddedLamp_ctl.tx_buff) {
		kfree(embeddedLamp_ctl.tx_buff);
		embeddedLamp_ctl.tx_buff = 0;
	}

	return error;
}



/**
 * Callbacks for the linux kernel driver
 */
static const struct file_operations embeddedLamp_fops = {
	.owner =	THIS_MODULE,
	.read = 	embeddedLamp_read,
	.write = 	embeddedLamp_write,
	.open =	embeddedLamp_open,	
};



/**
 * Initializes the character device (will be called in initialization of this kernel module)
 */
static int __init embeddedLamp_init_cdev(void)
{
	int error;

	embeddedLamp_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&embeddedLamp_dev.devt, 0, 1, this_driver_name);
	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", 
			error);
		return -1;
	}

	cdev_init(&embeddedLamp_dev.cdev, &embeddedLamp_fops);
	embeddedLamp_dev.cdev.owner = THIS_MODULE;
	
	error = cdev_add(&embeddedLamp_dev.cdev, embeddedLamp_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(embeddedLamp_dev.devt, 1);
		return -1;
	}	

	return 0;
}



/**
 * Registers this module in the linux kernel
 */
static int __init embeddedLamp_init_class(void)
{
	embeddedLamp_dev.class = class_create(THIS_MODULE, this_driver_name);

	if (!embeddedLamp_dev.class) {
		printk(KERN_ALERT "class_create() failed\n");
		return -1;
	}

	if (!device_create(embeddedLamp_dev.class, NULL, embeddedLamp_dev.devt, NULL, 	
			this_driver_name)) {
		printk(KERN_ALERT "device_create(..., %s) failed\n",
			this_driver_name);
		class_destroy(embeddedLamp_dev.class);
		return -1;
	}

	return 0;
}


/**
 * Initialization of the kernel module
 */
static int __init embeddedLamp_init(void)
{
	memset(&embeddedLamp_dev, 0, sizeof(embeddedLamp_dev));
	memset(&embeddedLamp_ctl, 0, sizeof(embeddedLamp_ctl));

	spin_lock_init(&embeddedLamp_dev.spi_lock);
	sema_init(&embeddedLamp_dev.fop_sem, 1);
	
	// initalize the character device
	if (embeddedLamp_init_cdev() < 0) 
		goto fail_1;
	
	// register this module with the linux kernel
	if (embeddedLamp_init_class() < 0)  
		goto fail_2;

	// initialize SPI bus
	if (embeddedLamp_init_spi() < 0) 
		goto fail_3;

	/* enforce some range to the write frequency, this is arbitrary */
	if (write_frequency < 1 || write_frequency > 10000) {
		printk(KERN_ALERT "write_frequency reset to %d", 
			DEFAULT_WRITE_FREQUENCY);

		write_frequency = DEFAULT_WRITE_FREQUENCY;
	}

	// set timing of transimissions
	if (write_frequency == 1)
		embeddedLamp_dev.timer_period_sec = 1;
	else
		embeddedLamp_dev.timer_period_ns = NANOSECS_PER_SEC / write_frequency; 

	hrtimer_init(&embeddedLamp_dev.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	embeddedLamp_dev.timer.function = embeddedLamp_timer_callback;
	/* leave the embeddedLamp_dev.timer.data field = NULL, not needed */

	return 0;

	
	
// initialization of SPI failed
fail_3:
	device_destroy(embeddedLamp_dev.class, embeddedLamp_dev.devt);
	class_destroy(embeddedLamp_dev.class);

// registration of the module failed
fail_2:
	cdev_del(&embeddedLamp_dev.cdev);
	unregister_chrdev_region(embeddedLamp_dev.devt, 1);

// initialization of character device failed
fail_1:
	return -1;
}
module_init(embeddedLamp_init);



/**
 * This function is being called when the kernel unloads the module
 */
static void __exit embeddedLamp_exit(void)
{

	gpio_free(GPIO_PIN);
	spi_unregister_device(embeddedLamp_dev.spi_device);
	spi_unregister_driver(&embeddedLamp_driver);

	// remove the character device from the file system
	device_destroy(embeddedLamp_dev.class, embeddedLamp_dev.devt);
	
	// unregister module
	class_destroy(embeddedLamp_dev.class);

	cdev_del(&embeddedLamp_dev.cdev);
	unregister_chrdev_region(embeddedLamp_dev.devt, 1);

	if (embeddedLamp_ctl.tx_buff)
		kfree(embeddedLamp_ctl.tx_buff);

	if (embeddedLamp_dev.user_buff)
		kfree(embeddedLamp_dev.user_buff);
}
module_exit(embeddedLamp_exit);

MODULE_AUTHOR("Bartholomaeus Dedersen");
MODULE_DESCRIPTION("embeddedLamp");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");

