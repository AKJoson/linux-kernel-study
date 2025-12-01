## 如何配置设备树与注册中断处理函数？

* 设备树编译的命令dtc
```c
dtc -I dts -O dtb -o temp.dtbo source.dts
```
* 动态加载设备树
```c
sudo dtoverlay temp.dtbo
```
* 查询动态加载的设备树
```c
dtoverlay -l
```
* 卸载已经加载的设备树
```c
sudo dtoverlay -R temp
```

###  platform_driver_register的出现解决什么问题？
让外设的配置变得灵活，相同的驱动程序，可以处理不同的外设。而不必修改或重写驱动程序。

### gpio相关的API
```
/*初始化*/
gpio_desc *my_led = gpiod_get(dev, "led", GPIOD_OUT_LOW);
/*设置值*/
gpiod_set_value(my_led, 1);
/*获取值*/
int led_val;
led_val = gpiod_get_value(my_led);
/*从gpio获取中断号*/
irq = gpiod_to_irq(my_touch);
/*释放*/
gpiod_put(my_touch);
```

