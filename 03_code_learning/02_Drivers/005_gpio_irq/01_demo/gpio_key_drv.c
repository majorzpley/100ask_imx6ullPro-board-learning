#include "asm/gpio.h"
#include "linux/device.h"
#include "linux/gpio.h"
#include "linux/init.h"
#include "linux/interrupt.h"
#include "linux/irqreturn.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/of.h"
#include "linux/of_gpio.h"
#include "linux/platform_device.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stddef.h"

struct gpio_key {
  int gpio;
  int irq;
  enum of_gpio_flags flags;
};

static struct gpio_key *gpio_keys_100ask;

static const struct of_device_id ask100_gpios[] = {
    {.compatible = "100ask,gpio_keys"},
    {},
};

static irqreturn_t gpio_key_irq_100ask(int irq, void *dev_id) {
  struct gpio_key *gpio_key = dev_id;

  printk("key %d val %d\n", irq, gpio_get_value(gpio_key->gpio));

  return IRQ_HANDLED;
}

static int chip_demo_gpio_probe(struct platform_device *pdev) {
  int count;
  int i;
  int gpio;
  int irq;
  int err;
  enum of_gpio_flags flag;
  struct device_node *node = pdev->dev.of_node;

  count = of_gpio_count(node);

  gpio_keys_100ask =
      kzalloc(sizeof(struct gpio_key) * count, GFP_KERNEL); // kernel zero alloc

  for (i = 0; i < count; i++) {
    gpio = of_get_gpio_flags(node, i, &flag);
    irq = gpio_to_irq(gpio);

    gpio_keys_100ask[i].gpio = gpio;
    gpio_keys_100ask[i].irq = irq;
    gpio_keys_100ask[i].flags = flag;

    err = request_irq(irq, gpio_key_irq_100ask,
                      IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
                      "100ask_gpio_key", &gpio_keys_100ask[i]);
  }

  return 0;
}
static int chip_demo_gpio_remove(struct platform_device *pdev) {
  struct device_node *node = pdev->dev.of_node;
  int count;
  int i;

  count = of_gpio_count(node);

  for (i = 0; i < count; i++) {
    free_irq(gpio_keys_100ask[i].irq, &gpio_keys_100ask[i]);
  }

  kfree(gpio_keys_100ask);

  return 0;
}

/*
 *1. 定义platform_driver
 */
static struct platform_driver chip_demo_gpio_driver = {
    .probe = chip_demo_gpio_probe,
    .remove = chip_demo_gpio_remove,
    .driver =
        {
            .name = "100ask_gpio_key",
            .of_match_table = ask100_gpios,
        },
};

static int __init test_gpio_drv_init(void) {
  platform_driver_register(&chip_demo_gpio_driver);
  return 0;
}

static void __exit test_gpio_drv_exit(void) {
  platform_driver_unregister(&chip_demo_gpio_driver);
}

module_init(test_gpio_drv_init);
module_exit(test_gpio_drv_exit);
MODULE_LICENSE("GPL");