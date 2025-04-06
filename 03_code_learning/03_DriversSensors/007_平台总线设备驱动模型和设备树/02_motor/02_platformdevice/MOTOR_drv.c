// todo platform总线(搭配设备树)设备驱动模板：使用platform_device来获取硬件资源
//  GPIO0:GPIO4_19,编号:3 * 32 +19 = 115
//  GPIO1:GPIO4_20,编号:3 * 32 +20 = 116
//  GPIO2:GPIO4_21,编号:3 * 32 +21 = 117
//  GPIO3:GPIO4_22,编号:3 * 32 +22 = 118
//  在"100ask_imx6ull-14x14.dts"中加入
/*
motor {
  compatible = "100ask,gpiodemo";
  gpios = <&gpio4 19 GPIO_ACTIVE_HIGH>,,
          <&gpio4 20 GPIO_ACTIVE_HIGH>,
          <&gpio4 21 GPIO_ACTIVE_HIGH>,
          <&gpio4 22 GPIO_ACTIVE_HIGH>;
};
*/
/*  头文件-------------------------------------------*/
#include "asm-generic/errno-base.h"
#include "asm/gpio.h"
#include "linux/delay.h"
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
#include "linux/uaccess.h"

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

static int g_s_motor_pin_ctrl[8] = {0x02, 0x03, 0x01, 0x09, 0x08,
                                    0x0c, 0x04, 0x06}; /*马达引脚设置数组*/
static int g_s_motor_index = 0;

/*  自定义函数---------------------------------------*/
static void set_pins_for_motor(int index) {
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(g_s_p_gpios[i].gpio,
                   g_s_motor_pin_ctrl[index] & (1 << i) ? 1 : 0);
  }
}

/*将所有引脚置于高阻态*/
static void disable_motor(void) {
  int i;
  for (i = 0; i < 4; i++) {
    gpio_set_value(g_s_p_gpios[i].gpio, 0);
  }
}

/*  中断处理函数-------------------------------------*/
static void timer_expire(unsigned long data) {}
static irqreturn_t top_isr(int fd, void *data) { return IRQ_HANDLED; }

/*  file_operations接口函数--------------------------*/
/*
 * int buf[2]
 * buf[0] = 步进的次数，> 0：逆时针步进，< 0：顺时针步进
 * buf[1] = mdelay的时间(调速)
 */
static ssize_t dev_drv_write(struct file *fp, const char __user *buf,
                             size_t size, loff_t *offset) {
  int kernel_buf[2];
  int err;
  int step;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (size != 8) {
    return -EINVAL;
  }

  err = copy_from_user(kernel_buf, buf, size);

  if (kernel_buf[0] > 0) {
    /*逆时针旋转*/
    for (step = 0; step < kernel_buf[0]; step++) {
      set_pins_for_motor(g_s_motor_index);
      mdelay(kernel_buf[1]);
      g_s_motor_index--;
      if (g_s_motor_index == -1) {
        g_s_motor_index = 7;
      }
    }
  } else {
    /*顺时针旋转*/
    kernel_buf[0] = 0 - kernel_buf[0];
    for (step = 0; step < kernel_buf[0]; step++) {
      set_pins_for_motor(g_s_motor_index);
      mdelay(kernel_buf[1]);
      g_s_motor_index++;
      if (g_s_motor_index == 8) {
        g_s_motor_index = 0;
      }
    }
  }
  /*改进：选抓到位后，让马达不再消耗电源*/
  disable_motor();

  return size;
}

/*  file_operations定义------------------------------*/
static struct file_operations g_s_fops = {
    .owner = THIS_MODULE,
    .write = dev_drv_write,
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
  /*申请GPIO资源*/
  for (i = 0; i < g_count; i++) {
    err = gpio_request(g_s_p_gpios[i].gpio, g_s_p_gpios[i].name);
    /*输出低电平，电机ABCD开路高阻态，防止电机过热*/
    gpio_direction_output(g_s_p_gpios[i].gpio, 0);
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
                "MOTOR"); /*/dev/MOTOR*/
  return err;
}

static int platform_drv_remove(struct platform_device *pdev) {
  int i;

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  device_destroy(g_s_p_class, MKDEV(g_major, 0)); /*销毁设备*/
  class_destroy(g_s_p_class);                     /*销毁类*/
  unregister_chrdev(g_major, "dev_major");        /*卸载驱动程序*/

  for (i = 0; i < g_count; i++) {
    gpio_free(g_s_p_gpios[i].gpio);
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
    {},
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
