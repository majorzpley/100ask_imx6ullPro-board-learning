#include "asm/uaccess.h"
#include "linux/device.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/gpio/consumer.h"
#include "linux/init.h"
#include "linux/kdev_t.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/platform_device.h"
#include "linux/printk.h"
#include "linux/stddef.h"

/*1.确定主设备号*/
static int major = 0;
static struct class *led_class;
static struct gpio_desc *led_gpio;

static int led_drv_open(struct inode *node, struct file *file) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  /* 根据次设备号初始化LED为熄灭状态*/
  gpiod_direction_output(led_gpio, 0);
  return 0;
}

static int led_drv_close(struct inode *node, struct file *file) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  return 0;
};

ssize_t led_drv_read(struct file *file, char __user *buf, size_t len,
                     loff_t *offset) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  return 0;
}

/*write(fd,&val,1);*/
ssize_t led_drv_write(struct file *file, const char __user *buf, size_t len,
                      loff_t *offset) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  char status;

  int err = copy_from_user(&status, buf, 1);

  /*根据次设备号和status控制led，status表示逻辑值，1亮0灭*/
  gpiod_set_value(led_gpio, status);

  return 1;
}

/*定义自己的file_operations结构体*/
static struct file_operations led_drv = {
    .owner = THIS_MODULE,
    .open = led_drv_open,
    .read = led_drv_read,
    .write = led_drv_write,
    .release = led_drv_close,
};

/*4.从platform_device中获得GPIO，把file_operations结构体告诉内核：注册驱动程序*/
static int chip_demo_gpio_probe(struct platform_device *pdev) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  int err;

  /*4.1 设备树文件中定义有: led-gpios = <...>;*/
  led_gpio = gpiod_get(&pdev->dev, "led", 0);
  if (IS_ERR(led_gpio)) {
    dev_err(&pdev->dev, "Failed to get GPIO for led\n");
    return PTR_ERR(led_gpio);
  }

  /*4.2 注册file_operations*/
  major = register_chrdev(0, "majorzpley_led", &led_drv);

  led_class = class_create(THIS_MODULE, "majorzpley_led_class");
  if (IS_ERR(led_class)) {
    unregister_chrdev(major, "led");
    gpiod_put(led_gpio);
    return PTR_ERR(led_class);
  }

  device_create(led_class, NULL, MKDEV(major, 0), NULL, "majorzpley_led%d", 0);

  return 0;
}

static int chip_demo_gpio_remove(struct platform_device *pdev) {
  device_destroy(led_class, MKDEV(major, 0));
  class_destroy(led_class);
  unregister_chrdev(major, "majorzpley_led");
  gpiod_put(led_gpio);
  return 0;
}

static const struct of_device_id majorzpley_leds[] = {
    {.compatible = "majorzpley,led_drv"},
    {},
};

/*1. 定义platform_driver*/
static struct platform_driver chip_demo_gpio_driver = {
    .probe = chip_demo_gpio_probe,
    .remove = chip_demo_gpio_remove,
    .driver =
        {
            .name = "majorzpley_led",
            .of_match_table = majorzpley_leds,
        },
};

/*2.在入口函数注册platform_drver*/
static int __init led_init(void) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  int err;
  err = platform_driver_register(&chip_demo_gpio_driver);
  return err;
}
/*3.在出口函数卸载platform_driver*/
static void __exit led_exit(void) {
  printk("file %s function %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  platform_driver_unregister(&chip_demo_gpio_driver);
}

/*7. 其他完善：提供设备信息，自动创建设备节点*/
module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");