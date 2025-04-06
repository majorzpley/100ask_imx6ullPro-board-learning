// todo platform总线(搭配设备树)设备驱动模板
/*  头文件-------------------------------------------*/
#include "asm-generic/errno-base.h"
#include "asm/gpio.h"
#include "linux/device.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/gfp.h"
#include "linux/init.h"
#include "linux/interrupt.h"
#include "linux/ioport.h"
#include "linux/irqreturn.h"
#include "linux/kdev_t.h"
#include "linux/kernel.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/of.h"
#include "linux/of_gpio.h"
#include "linux/platform_device.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/stddef.h"
#include "linux/timer.h"

/*  自定义宏-----------------------------------------*/
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DEV: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

/*  变量---------------------------------------------*/
struct gpio_describe {
  int gpio;
  int irq;
  char name[128];
  int data;
  struct timer_list gpio_timer;
};
static struct gpio_describe *g_s_p_gpios;
static int g_major;
static struct class *g_s_p_class;
static int g_count;

/*  自定义函数---------------------------------------*/
/*  中断处理函数-------------------------------------*/
static void timer_expire(unsigned long data) {}
static irqreturn_t top_isr(int fd, void *data) { return IRQ_HANDLED; }

/*  file_operations接口函数--------------------------*/
/*  file_operations定义------------------------------*/
static struct file_operations g_s_fops = {
    .owner = THIS_MODULE,
};

/*  platform_driver接口函数--------------------------*/
static int platform_drv_probe(struct platform_device *pdev) {
  struct device_node *np = pdev->dev.of_node;
  int i;
  struct resource *res;
  int err = 0;
  /*从platform_device获得引脚信息
   *1.pdev来自c文件
   *2.pdev来自设备树of_node
   */
  if (np) {
    /*pdev来自设备树：示例
    reg_usb_ltemodule: regulator@1 {
        compatible = "100ask,gpio_demo";
        gpios = <&gpio5 5 GPIO_ACTIVE_HIGH>， <&gpio5 3 GPIO_ACTIVE_HIGH>;
    };
     */
    g_count = of_gpio_count(np);
    if (!g_count)
      return -EINVAL;
    g_s_p_gpios = kmalloc(g_count * sizeof(struct gpio_describe), GFP_KERNEL);
    for (i = 0; i < g_count; i++) {
      g_s_p_gpios[i].gpio = of_get_gpio(np, i);
      sprintf(g_s_p_gpios[i].name, "%s_pin_%d", np->name, i);
    }
  } else {
    /*pdev来自c文件：示例
    static struct resource omap16xx_gpio3_resources[] = {
      {
        .start = 115,
        .end = 115,
        .flags = IORESOURSE_IRQ
      },
      {
        .start = 116,
        .end = 116,
        .flags = IORESOURSE_IRQ
      },
    }
 */
    g_count = 0;
    while (1) {
      res = platform_get_resource(pdev, IORESOURCE_IRQ, g_count);
      if (res) {
        g_count++;
      } else {
        break;
      }
    }
    if (!g_count)
      return -EINVAL;
    g_s_p_gpios = kmalloc(g_count * sizeof(struct gpio_describe), GFP_KERNEL);
    for (i = 0; i < g_count; i++) {
      res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
      g_s_p_gpios[i].gpio = res->start;
      sprintf(g_s_p_gpios[i].name, "%s_pin_%d", pdev->name, i);
    }
  }
  for (i = 0; i < g_count; i++) {
    g_s_p_gpios[i].irq = gpio_to_irq(g_s_p_gpios[i].gpio);
    setup_timer(&g_s_p_gpios[i].gpio_timer, timer_expire,
                (unsigned long)&g_s_p_gpios[i]);
    g_s_p_gpios[i].gpio_timer.expires = ~0;
    add_timer(&g_s_p_gpios[i].gpio_timer);
    err = request_irq(g_s_p_gpios[i].irq, top_isr,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                      g_s_p_gpios[i].name, &g_s_p_gpios[i]);
  }

  /*注册file_operations*/
  g_major = register_chrdev(0, "dev_major", &g_s_fops);
  g_s_p_class = class_create(THIS_MODULE, "dev_class");
  if (IS_ERR(g_s_p_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_major, "dev_major");
    return PTR_ERR(g_s_p_class);
  }
  device_create(g_s_p_class, NULL, MKDEV(g_major, 0), NULL,
                "template_dev"); /*/dev/template_dev*/
  return err;
}

static int platform_drv_remove(struct platform_device *pdev) {
  int i;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_p_class, MKDEV(g_major, 0)); /*销毁设备*/
  class_destroy(g_s_p_class);                     /*销毁类*/
  unregister_chrdev(g_major, "dev_major");        /*卸载驱动程序*/

  for (i = 0; i < g_count; i++) {
    free_irq(g_s_p_gpios[i].irq, &g_s_p_gpios[i]);
    del_timer(&g_s_p_gpios[i].gpio_timer);
  }

  kfree(g_s_p_gpios);
  g_s_p_gpios = NULL;

  return 0;
}

/*  platform_driver定义------------------------------*/
static const struct of_device_id g_s_dt_ids[] = {
    {
        .compatible = "100ask,gpio_demo",
    },
};
static struct platform_driver g_s_platform_drv = {
    .driver =
        {
            .name = "template_platform_drv",
            .of_match_table = g_s_dt_ids,
        },
    .probe = platform_drv_probe,
    .remove = platform_drv_remove,
};

/*  入口函数-----------------------------------------*/
static int __init dev_drv_init(void) {
  /*注册platform_driver*/
  return platform_driver_register(&g_s_platform_drv);
}

/*  出口函数-----------------------------------------*/
static void __exit dev_drv_exit(void) {
  /*卸载platform_driver*/
  platform_driver_unregister(&g_s_platform_drv);
}

/*  注册入口出口-------------------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");
