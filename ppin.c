/*
 *		Parallel Port Pin driver for Linux 2.6: ppin
 *
 *		This kernel module will register the /dev/ppin (10, 151)
 *		device which controls up to eight pins through the first
 *		parallel port.
 *
 *		Controlling the pins is as easy as 'echo Num State >/dev/ppin',
 *		where Num is 0 to 7 and State is one of 'on', 'off'.
 *		For example: "echo 3 on >/dev/ppin" switches the 3rd pin on.
 *
 *		You can read the status of the pins with 'cat /dev/ppin'.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexander Barton, <alex@barton.de> (for Linux 2.6, 2009)
 *
 *		This work is heavily(!) based on the "devled" driver written by
 *		Konstantinos Natsakis, <cyfex@mail.com> (for Linux 2.2/2.4).
 */

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/parport.h>
#include <asm/uaccess.h>

#define PPIN_NAME	"ppin"
#define PPIN_VERSION	"0.1"

#define PPIN_DEV	"ppin"
#define PPIN_MAJOR	MISC_MAJOR
#define PPIN_MINOR	151

#define ON_COMMAND	"on"
#define OFF_COMMAND	"off"

static char pin_state = 0;
static int buffer_empty = 0;
static int ppin_open_cnt = 0;
static int available_ports = 0;
static struct pardevice *parport_pins = 0;

MODULE_AUTHOR("Alexander Barton, alex@barton.de");
MODULE_DESCRIPTION("Driver for controlling the state of parallel port PINs");
MODULE_LICENSE("GPL");

void
set_pins(void)
{
	if (parport_claim_or_block(parport_pins) < 0) {
		printk(KERN_ERR
		       "Could not claim the " PPIN_DEV " parallel port device\n");
		return;
	}
	parport_write_data(parport_pins->port, pin_state);
	parport_release(parport_pins);
}

static void
ppin_attach(struct parport *port)
{
	if (available_ports == 0) {
		parport_pins =
		    parport_register_device(port, PPIN_DEV, NULL, NULL,
					    NULL, 0, 0);

		if (parport_pins == 0)
			printk(KERN_ERR
			       "Could not associate " PPIN_DEV " device with parallel port #%d\n",
			       available_ports + 1);
		else
			printk(KERN_INFO
			       "Associated " PPIN_DEV " device with parallel port #%d\n",
			       available_ports + 1);
	}

	available_ports++;
}

static void
ppin_detach(struct parport *port)
{
	if (available_ports == 1)
		parport_pins = 0;

	available_ports--;
}

static struct parport_driver ppin_driver = {
	PPIN_NAME,
	ppin_attach,
	ppin_detach,
	{NULL}
};

static ssize_t
ppin_read(struct file *file, char *buf, size_t count, loff_t * ppos)
{
	int i;
	char *tmp = buf;
	char status[79];

	if (buffer_empty)
		return 0;

	sprintf(&status[0],
		"PIN #:   0   1   2   3   4   5   6   7\n"
		"State: %s %s %s %s %s %s %s %s\n",
		pin_state & (1 << 0) ? " on" : "off",
		pin_state & (1 << 1) ? " on" : "off",
		pin_state & (1 << 2) ? " on" : "off",
		pin_state & (1 << 3) ? " on" : "off",
		pin_state & (1 << 4) ? " on" : "off",
		pin_state & (1 << 5) ? " on" : "off",
		pin_state & (1 << 6) ? " on" : "off",
		pin_state & (1 << 7) ? " on" : "off");

	for (i = 0; count-- > 0 && i < 78; ++i, ++tmp)
		put_user(status[i], tmp);

	if (tmp - buf > 77)
		buffer_empty = 1;

	return (tmp - buf);
}

static ssize_t
ppin_write(struct file *file, const char *buf, size_t count, loff_t * ppos)
{
	int i;
	char *tmp_1, *tmp_2;

	i = simple_strtol(buf, &tmp_2, 0);

	if (i < 0 || i > 7) {
		printk(KERN_WARNING "" PPIN_DEV ": No such PIN: %d\n", i);
		return count;
	}
	tmp_2++;

	tmp_1 = strstr(tmp_2, OFF_COMMAND);
	if (tmp_1 != NULL)
		pin_state &= ~(1 << i);
	else {
		tmp_1 = strstr(tmp_2, ON_COMMAND);
		if (tmp_1 != NULL)
			pin_state |= (1 << i);
	}

	if (!tmp_1) {
		printk(KERN_WARNING "" PPIN_DEV": No such state\n");
		return count;
	}

	set_pins();
	return count;
}

static int
ppin_open(struct inode *inode, struct file *file)
{
	if (ppin_open_cnt)
		return -EBUSY;
	else
		ppin_open_cnt = 1;

	buffer_empty = 0;
	return 0;
}

static int
ppin_release(struct inode *inode, struct file *file)
{
	ppin_open_cnt = 0;
	return 0;
}

static struct file_operations ppin_fops = {
	owner:THIS_MODULE,
	read:ppin_read,
	write:ppin_write,
	open:ppin_open,
	release:ppin_release,
};

static struct miscdevice ppin_dev = {
	PPIN_MINOR,
	PPIN_DEV,
	&ppin_fops
};

int __init
ppin_init(void)
{
	if (parport_register_driver(&ppin_driver) != 0) {
		printk(KERN_ERR "Could not register the " PPIN_DEV " driver.\n");
		return -EIO;
	}

	if (misc_register(&ppin_dev) != 0) {
		printk(KERN_ERR
		       "Could not register the misc device " PPIN_DEV " (%d, %d)\n",
		       PPIN_MAJOR, PPIN_MINOR);
		return -EIO;
	}

	printk(KERN_INFO "" PPIN_NAME " driver v%s loaded\n", PPIN_VERSION);

	set_pins();
	return 0;
}

static void __exit
ppin_cleanup(void)
{
	if (misc_deregister(&ppin_dev) != 0)
		printk(KERN_ERR
		       "Cound not deregister the misc device " PPIN_DEV " (%d, %d)\n",
		       PPIN_MAJOR, PPIN_MINOR);

	parport_unregister_device(parport_pins);
	parport_unregister_driver(&ppin_driver);

	printk(KERN_INFO "" PPIN_NAME " driver v%s unloaded\n", PPIN_VERSION);
}

module_init(ppin_init);
module_exit(ppin_cleanup);
