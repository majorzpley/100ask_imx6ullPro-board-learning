#include "led_resource.h"
#include "linux/device.h"
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>

static struct resource board_A_led_resource[] = {
    {
        .start = GROUP_PIN(5, 3),
        .flags = IORESOURCE_IRQ,
        .name = "majorzpley_led_pin",
    },
};

static void led_dev_release(struct device *dev) { printk("led_dev_release\n"); }

static struct platform_device board_A_led_dev = {
    .resource = board_A_led_resource,
    .name = "majorzpley_led",
    .num_resources = ARRAY_SIZE(board_A_led_resource),
    .dev = {
        .release = led_dev_release,
    }};

static int __init led_dev_init(void) {
  int err;
  err = platform_device_register(&board_A_led_dev);
  return 0;
}

static void __exit led_dev_exit(void) {
  platform_device_unregister(&board_A_led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");