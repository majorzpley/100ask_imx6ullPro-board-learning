// todo platform总线device设备驱动模板：使用platform_device来获取硬件资源
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

/*  自定义函数---------------------------------------*/

/*  中断处理函数-------------------------------------*/

/*  platform_device定义------------------------------*/
static struct resource g_p_resource[] = {
    {
        .start = 115,
        .end = 115,
        .flags = IORESOURCE_IRQ,
    },
    {
        .start = 116,
        .end = 116,
        .flags = IORESOURCE_IRQ,
    },
    {
        .start = 117,
        .end = 117,
        .flags = IORESOURCE_IRQ,
    },
    {
        .start = 118,
        .end = 118,
        .flags = IORESOURCE_IRQ,
    },
};
static struct platform_device g_s_dev = {
    .name = "template_platform_drv",
    .id = 0,
    .num_resources = ARRAY_SIZE(g_p_resource),
    .resource = g_p_resource,
};

/*  入口函数-----------------------------------------*/
static int __init platform_dev_init(void) {
  /*注册platform_device*/
  return platform_device_register(&g_s_dev);
}

/*  出口函数-----------------------------------------*/
static void __exit platform_dev_exit(void) {
  /*卸载platform_device*/
  platform_device_unregister(&g_s_dev);
}

/*  注册入口出口-------------------------------------*/
module_init(platform_dev_init);
module_exit(platform_dev_exit);
MODULE_LICENSE("GPL");
