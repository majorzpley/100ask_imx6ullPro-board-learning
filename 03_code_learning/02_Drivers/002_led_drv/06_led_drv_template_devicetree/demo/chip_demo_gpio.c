#include "led_drv.h"
#include "led_opr.h"
#include "led_resource.h"
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/tty.h>

static int g_ledpins[100];
static int g_ledpins_cnt = 0;

static int board_demo_led_init(int which) /* 初始化LED, which-哪个LED */
{
  printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);
  printk("init gpio: group %d, pin %d\n", GROUP(g_ledpins[which]),
         PIN(g_ledpins[which]));
  switch (GROUP(g_ledpins[which])) {
  case 0:
    printk("init pin of group 0 ...\n");
    break;
  case 1:
    printk("init pin of group 1 ...\n");
    break;
  case 2:
    printk("init pin of group 2 ...\n");
    break;
  case 3:
    printk("init pin of group 3 ...\n");
    break;
  }
  return 0;
}

/* 控制LED, which-哪个LED, status:1-亮,0-灭 */
static int board_demo_led_ctl(int which, char status) {
  printk("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__, which,
         status ? "on" : "off");

  printk("set led %s: group %d, pin %d\n", status ? "on" : "off",
         GROUP(g_ledpins[which]), PIN(g_ledpins[which]));

  switch (GROUP(g_ledpins[which])) {
  case 0:
    printk("set pin of group 0 ...\n");
    break;
  case 1:
    printk("set pin of group 1 ...\n");
    break;
  case 2:
    printk("set pin of group 2 ...\n");
    break;
  case 3:
    printk("set pin of group 3 ...\n");
    break;
  }
  return 0;
}

static struct led_operations board_demo_led_opr = {
    .init = board_demo_led_init,
    .ctl = board_demo_led_ctl,
};

//> 此函数会调用两次
static int chip_demo_gpio_probe(struct platform_device *pdev) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  int err = 0;
  struct device_node *np;
  int led_pin;

  np = pdev->dev.of_node;
  if (!np) {
    return -1;
  }

  err = of_property_read_u32(np, "pin", &led_pin);

  /*
   *记录引脚
   */
  g_ledpins[g_ledpins_cnt] = led_pin;
  /*
   *device_create
   */
  led_class_device_create(g_ledpins_cnt);
  g_ledpins_cnt++;

  return 0;
}

static int chip_demo_gpio_remove(struct platform_device *pdev) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  int i;
  struct device_node *np;
  int led_pin;
  int err;

  np = pdev->dev.of_node;
  if (!np) {
    return -1;
  }
  err = of_property_read_u32(np, "pin", &led_pin);

  /*
   *device_destroy
   */
  for (i = 0; i < g_ledpins_cnt; i++) {
    if (g_ledpins[i] == led_pin) {
      led_class_device_destroy(i);
      g_ledpins[i] = -1;
      break;
    }
  }

  for (i = 0; i < g_ledpins_cnt; i++) {
    if (g_ledpins[i] != -1)
      break;
  }
  /*
   *所有项都等于-1了
   */
  if (i == g_ledpins_cnt) {
    g_ledpins_cnt = 0;
  }
  return 0;
}

static const struct of_device_id majorzpley_leds[] = {
    {.compatible = "majorzpley,led_drv"},
    {},
};

static struct platform_driver chip_demo_gpio_drv = {
    .probe = chip_demo_gpio_probe,
    .remove = chip_demo_gpio_remove,
    .driver =
        {
            .name = "majorzpley_led",
            .owner = THIS_MODULE,
            .of_match_table = majorzpley_leds,
        },
};

static int __init chip_demo_gpio_drv_init(void) {
  int err;
  err = platform_driver_register(&chip_demo_gpio_drv);
  register_led_operations(&board_demo_led_opr);
  return 0;
}

static void __exit chip_demo_gpio_drv_exit(void) {
  platform_driver_unregister(&chip_demo_gpio_drv);
}

module_init(chip_demo_gpio_drv_init);
module_exit(chip_demo_gpio_drv_exit);
MODULE_LICENSE("GPL");