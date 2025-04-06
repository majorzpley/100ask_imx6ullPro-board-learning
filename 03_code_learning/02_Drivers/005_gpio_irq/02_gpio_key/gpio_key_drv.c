//? 可以参考kernel中drivers/input/keyboard/gpio_key.c来写
#include "asm/gpio.h"
#include "linux/gfp.h"
#include "linux/gpio.h"
#include "linux/gpio/consumer.h"
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
  struct gpio_desc *gpiod;
  int flag;
  int irq;
};

static struct gpio_key *gpio_key_100ask;

static irqreturn_t gpio_key_isr(int irq, void *dev_id) {
  struct gpio_key *key = (struct gpio_key *)dev_id;
  int val;
  val = gpiod_get_value(key->gpiod);

  printk("key %d %d\n", key->gpio, val);

  return IRQ_HANDLED;
}

/*
 * 1.从platform_device获得GPIO
 * 2.gpio=>irq
 * 3.request_irq
 */
static int gpio_key_probe(struct platform_device *pdev) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  int err;
  struct device_node *node = pdev->dev.of_node;
  int count;
  int i;
  enum of_gpio_flags flag;
  unsigned flags = GPIOF_IN;

  // todo 获得节点个数
  count = of_gpio_count(node);
  if (!count) {
    printk("%s %s line %d, there isn't any gpio available\n", __FILE__,
           __FUNCTION__, __LINE__);
    return -1;
  }

  gpio_key_100ask = kzalloc(sizeof(struct gpio_key) * count, GFP_KERNEL);

  for (i = 0; i < count; i++) {
    gpio_key_100ask[i].gpio = of_get_gpio_flags(node, i, &flag);
    if (gpio_key_100ask[i].gpio < 0) {
      printk("%s %s line %d, of_get_gpio_flags fail\n", __FILE__, __FUNCTION__,
             __LINE__);
      return -1;
    }
    gpio_key_100ask[i].gpiod = gpio_to_desc(gpio_key_100ask[i].gpio);
    gpio_key_100ask[i].flag = flag & OF_GPIO_ACTIVE_LOW;

    //如果设备树里引脚配置为GPIO_ACTIVE_LOW，那么和引脚的物理值相反
    if (gpio_key_100ask[i].flag) {
      flags |= GPIOF_ACTIVE_LOW;
    }

    err =
        devm_gpio_request_one(&pdev->dev, gpio_key_100ask[i].gpio, flags, NULL);

    gpio_key_100ask[i].irq = gpio_to_irq(gpio_key_100ask[i].gpio);
  }

  for (i = 0; i < count; i++) {
    // 这里需要把gpio_key_100ask里的gpiod描述信息传参至isr中断函数中获取管教电平信息
    err = request_irq(gpio_key_100ask[i].irq, gpio_key_isr,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      "100ask_gpio_keys", &gpio_key_100ask[i]);
  }
  return 0;
}

static int gpio_key_remove(struct platform_device *pdev) {
  struct device_node *node = pdev->dev.of_node;
  int count;
  int i;

  count = of_gpio_count(node);

  for (i = 0; i < count; i++) {
    //释放中断资源
    free_irq(gpio_key_100ask[i].irq, &gpio_key_100ask[i]);
  }
  kfree(gpio_key_100ask);
  return 0;
}

// 注意:这个的compatible字段需要和设备中节点中compatible属性一致
static struct of_device_id ask100_keys[] = {
    {.compatible = "100ask,gpio_keys"},
    {},
};

/*1.定义platform_driver*/
static struct platform_driver gpio_key_driver = {
    .probe = gpio_key_probe,
    .remove = gpio_key_remove,
    .driver =
        {
            .name = "100ask_gpio_keys",
            .of_match_table = ask100_keys,
        },
};

/*2.在入口函数注册platform_driver*/
static int __init gpio_key_init(void) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  int err;

  err = platform_driver_register(&gpio_key_driver);

  return err;
}

/*3.在出口函数卸载platform_driver*/
static void __exit gpio_key_exit(void) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  platform_driver_unregister(&gpio_key_driver);
}

module_init(gpio_key_init);
module_exit(gpio_key_exit);
MODULE_LICENSE("GPL");