#include "chip_imx6ull.h"
#include "led_drv.h"
#include "led_opr.h"
#include "led_resource.h"
#include <asm/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/stddef.h>
#include <linux/types.h>

static int g_ledpins[100];
static int g_ledpins_cnt = 0;

static volatile CCM_TypeDef *CCM;
static volatile IOMUXC_SNVS_TypeDef *IOMUXC_SNVS;
static volatile GPIOx_TypeDef *GPIO5;

static int board_led_init(int which) /* 初始化LED, which-哪个LED */
{
  uint32_t val = 0;

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
  case 4:
    printk("init pin of group 4 ...\n");
    break;
  case 5:
    printk("init pin of group 5 ...\n");
    if (!CCM) {
      CCM = ioremap(CCM_BASE_ADDR, sizeof(CCM_TypeDef));
      IOMUXC_SNVS = ioremap(IOMUXC_SNVS_BASE_ADDR, sizeof(IOMUXC_SNVS_TypeDef));
      GPIO5 = ioremap(GPIO5_BASE_ADDR, sizeof(GPIOx_TypeDef));
    }
    /*
     ? 1.使能GPIO5时钟：默认使能
      - set CCM ro enable GPIO5
      - CCM_CCGR1[CG15] 0x20c406c
      - bit[31:30] = 0b11 = 0x03
     */
    BIT_CLR(CCM->CCGR1, 30);
    BIT_CLR(CCM->CCGR1, 31);
    BIT_SET(CCM->CCGR1, 30);
    BIT_SET(CCM->CCGR1, 31);
    /*
     ? 2.设置GPIO5_IO3复用功能为GPIO：
      -当引脚被配置成输出模式时，若IOMUXC中的MUX寄存器使能了SION功能(输出通道回环至输入)
      -可以通过PSR 寄存器读取回引脚的状态值。
      - set IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 to configure as GPIO
      - IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 0x2290014
      - bit[4:0] = 0b10101 = 0x15
     */
    val = IOMUXC_SNVS->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
    val &= ~(0x1f);
    val |= 0x15;
    IOMUXC_SNVS->IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = val;

    /*
     ? 3.设置GPIO5_IO3的GPIO模式为output模式
      - set GPIO5_GDIR to configure GPIO5_IO3 as output
      - GPIO5_GDIR 0x020ac000 + 0x04
      - bit[3] = 0b1
     */
    BIT_SET(GPIO5->GDIR, 3);
    break;
  }
  return 0;
}

/* 控制LED, which-哪个LED, status:1-亮,0-灭 */
static int board_led_ctl(int which, char status) {
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
  case 4:
    printk("set pin of group 4 ...\n");
    break;
  case 5:
    printk("set pin of group 5 ...\n");
    if (status) {
      /*
       ? 5.设置GPIO5_IO3输出低电平
        - set GPIO5_DR to configure GPIO5_IO3 output 0
        - GPIO5_DR 0x020ac000 + 0x00
        - bit[3] = 0b0
       */
      BIT_CLR(GPIO5->DR, 3);
    } else {
      /*
       ? 4.设置GPIO5_IO3输出高电平
        - set GPIO5_DR to configure GPIO5_IO3 output 1
        - GPIO5_DR 0x020ac000 + 0x00
        - bit[3] = 0b1
       */
      BIT_SET(GPIO5->DR, 3);
    }

    break;
  }
  return 0;
}

static char board_led_getlevel(int which) {
  printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);

  char level = 0;
  int val;

  switch (GROUP(g_ledpins[which])) {
  case 0:
    printk("get pin of group 0 ...\n");
    break;
  case 1:
    printk("get pin of group 1 ...\n");
    break;
  case 2:
    printk("get pin of group 2 ...\n");
    break;
  case 3:
    printk("get pin of group 3 ...\n");
    break;
  case 4:
    printk("get pin of group 4 ...\n");
    break;
  case 5:
    printk("get pin of group 5 ...\n");
    val = GPIO5->DR;
    level = (val >> 3) & 0x1;
    break;
  }
  return level;
}

static int board_led_unmap(int which) {
  printk("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);
  switch (GROUP(g_ledpins[which])) {
  case 0:
    printk("unmap pin of group 0 ...\n");
    break;
  case 1:
    printk("unmap pin of group 1 ...\n");
    break;
  case 2:
    printk("unmap pin of group 2 ...\n");
    break;
  case 3:
    printk("unmap pin of group 3 ...\n");
    break;
  case 4:
    printk("unmap pin of group 4 ...\n");
    break;
  case 5:
    printk("unmap pin of group 5 ...\n");
    iounmap(CCM);
    iounmap(IOMUXC_SNVS);
    iounmap(GPIO5);
    //- 避免指针悬垂引用
    CCM = NULL;
    IOMUXC_SNVS = NULL;
    GPIO5 = NULL;
    break;
  }
  return 0;
}

static struct led_operations board_demo_led_opr = {
    .init = board_led_init,
    .ctl = board_led_ctl,
    .getLevel = board_led_getlevel,
    .unmap = board_led_unmap,
};

static int chip_demo_gpio_probe(struct platform_device *pdev) {
  int i = 0;
  struct resource *res;
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  while (1) {
    res = platform_get_resource(pdev, IORESOURCE_IRQ, i++);
    if (!res) {
      break;
    }
    /*
     *记录引脚
     */
    g_ledpins[g_ledpins_cnt] = res->start;
    /*
     *device_create
     */
    led_class_device_create(g_ledpins_cnt);
    g_ledpins_cnt++;
  }

  return 0;
}

static int chip_demo_gpio_remove(struct platform_device *pdev) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  struct resource *res;
  int i = 0;
  /*
   *device_destroy
   */
  while (1) {

    res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
    if (!res) {
      break;
    }
    led_class_device_destroy(i);
    i++;
    g_ledpins_cnt--;
  }
  return 0;
}

static struct platform_driver chip_demo_gpio_drv = {
    .probe = chip_demo_gpio_probe,
    .remove = chip_demo_gpio_remove,
    .driver =
        {
            .name = "majorzpley_led",
            .owner = THIS_MODULE,
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