#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>

static struct gpio_desc *led;

#define IO_LED 26

#define IO_OFFSET 569 // Raspberry Pi 5

static int __init my_init(void)
{
	int status;

	led = gpio_to_desc(IO_LED + IO_OFFSET);
	if (!led) {
		printk("gpioctrl - Error getting pin 26\n");
		return -ENODEV;
	}

	status = gpiod_direction_output(led, 0);
	if (status) {
		printk("gpioctrl - Error setting pin 26 to output\n");
		return status;
	}
	gpiod_set_value(led, 1);

	return 0;
}

static void __exit my_exit(void)
{
	gpiod_set_value(led, 0);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cherry");
MODULE_DESCRIPTION("An example for using GPIOs without the device tree");