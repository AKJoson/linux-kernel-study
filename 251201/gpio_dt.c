#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static struct gpio_desc *my_led = NULL;   // led灯的gpio
static struct gpio_desc *my_touch = NULL; // touch的gpio

#define TOUCH_DEBOUNCE_MS 50 // 50ms

static irqreturn_t touch_handle(int irq, void *dev_id) {
  pr_info("call - touch_handle\n");
  // 进入中断处理函数啦！！！
  static unsigned long last_jiffies = 0;
  if (time_after(jiffies, last_jiffies + msecs_to_jiffies(TOUCH_DEBOUNCE_MS))) {
    pr_info("handle touch\n");
    last_jiffies = jiffies;
    int led_val;
    led_val = gpiod_get_value(my_led);
    if (led_val) {
      gpiod_set_value(my_led, 0);
    } else {
      gpiod_set_value(my_led, 1);
    }
  }
  return IRQ_HANDLED;
}

static int my_probe(struct platform_device *ofdev) {
  int ret;
  // match
  pr_info("call - my_probe\n");
  // 取出led的gpio配置信息和touch的gpio配置信息
  struct device *dev = &ofdev->dev;
  const char *label;
  if (!device_property_present(dev, "label")) {
    pr_err("label can't found!\n");
    return -1;
  }
  if (!device_property_present(dev, "touch-gpios")) {
    pr_err("property touch-gpios can't found!\n");
    return -1;
  }
  if (!device_property_present(dev, "led-gpios")) {
    pr_err("property led-gpios can't found!\n");
    return -1;
  }
  ret = device_property_read_string(dev, "label", &label);
  if (ret) {
    pr_err("can't read label!\n");
    return -1;
  }
  pr_info("label value is %s\n", label);

  // init gpio

  my_led = gpiod_get(dev, "led", GPIOD_OUT_LOW);
  if (IS_ERR(my_led)) {
    pr_err("can't set up gpio led\n");
    return PTR_ERR(my_led);
  }
  pr_info("****turn on****\n");
  gpiod_set_value(my_led, 1);

  my_touch = gpiod_get(dev, "touch", GPIOD_OUT_HIGH);
  if (IS_ERR(my_touch)) {
    pr_err("can't set up gpio touch\n");
    return PTR_ERR(my_touch);
  }
  int irq;
  irq = gpiod_to_irq(my_touch);
  if (irq < 0) {
    pr_err("can't get my_touch irq\n");
    return irq;
  }
  pr_info("Get the touch irq value is %d\n", irq);
  // 按下 IRQF_TRIGGER_RISING |  松开 IRQF_TRIGGER_FALLING
  ret = devm_request_irq(dev, irq, touch_handle, IRQF_TRIGGER_FALLING,
                         "touch_button", dev);
  if (ret) {
    pr_err("failed to request irq\n");
    return ret;
  }
  return 0;
}

static void my_remove(struct platform_device *dev) {
  // 释放数据
  pr_info("call - my_remove\n");
  gpiod_set_value(my_led, 0);
  gpiod_put(my_led);
  gpiod_put(my_touch);
}

static const struct of_device_id gpio_match[] = {
    {
        .compatible =
            "yp,gpio_ctrl_dt", /*通过它与设备树里面的comptible来匹配，触发probe函数的回调*/
    },
    {},
};

static struct platform_driver my_gpio_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver =
        {
            .name = "testgpiodt",
            .of_match_table = gpio_match,
        },
};

static int __init gpio_ctrl_init(void) {
  pr_info("call - gpio_ctrl_init\n");
  int ret = platform_driver_register(&my_gpio_driver);
  return ret;
}

static void __exit gpio_ctrl_exit(void) {
  pr_info("call - gpio_ctrl_exit\n");
  platform_driver_unregister(&my_gpio_driver);
}

module_init(gpio_ctrl_init);
module_exit(gpio_ctrl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cherry");
MODULE_DESCRIPTION("A sample use the gpio&devicetree");