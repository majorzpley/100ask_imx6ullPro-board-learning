#include "led_opr.h"
#include "linux/stddef.h"
#include <asm/io.h>
#include <linux/printk.h>

static volatile unsigned int *CCM_CCGR1;
static volatile unsigned int *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
static volatile unsigned int *GPIO5_GDIR;
static volatile unsigned int *GPIO5_DR;
static volatile unsigned int *GPIO5_PSR;

// brief 初始化LED，which-哪个led
static int board_led_init(int which) {
  unsigned int val = 0;

  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  if (which == 0) {
    if (!CCM_CCGR1) {
      CCM_CCGR1 = ioremap(0x020c406c, 4); //按页映射
      IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(0x02290014, 4);
      GPIO5_GDIR = ioremap(0x020ac000 + 0x04, 4);
      GPIO5_DR = ioremap(0x020ac000 + 0x00, 4);
      GPIO5_PSR = ioremap(0x020ac000 + 0x08, 4);
    }
    /*
     ? 1.使能GPIO5时钟：默认使能
      - set CCM ro enable GPIO5
      - CCM_CCGR1[CG15] 0x20c406c
      - bit[31:30] = 0b11 = 0x03
     */
    *CCM_CCGR1 |= (3 << 30);
    /*
     ? 2.设置GPIO5_IO3复用功能为GPIO：
      -当引脚被配置成输出模式时，若IOMUXC中的MUX寄存器使能了SION功能(输出通道回环至输入)
      -可以通过PSR 寄存器读取回引脚的状态值。
      - set IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 to configure as GPIO
      - IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 0x2290014
      - bit[4:0] = 0b10101 = 0x15
     */
    val = *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;
    val &= ~(0x1f);
    val |= 0x15;
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = val;

    /*
     ? 3.设置GPIO5_IO3的GPIO模式为output模式
      - set GPIO5_GDIR to configure GPIO5_IO3 as output
      - GPIO5_GDIR 0x020ac000 + 0x04
      - bit[3] = 0b1
     */
    *GPIO5_GDIR |= (1 << 3);
  }
  return 0;
}

// brief 控制LED,which0哪个LED,status:1->亮,0->灭
static int board_led_ctl(int which, char status) {
  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
  if (which == 0) {
    if (status) {
      /*
       ? 5.设置GPIO5_IO3输出低电平
        - set GPIO5_DR to configure GPIO5_IO3 output 0
        - GPIO5_DR 0x020ac000 + 0x00
        - bit[3] = 0b0
       */
      *GPIO5_DR &= ~(1 << 3);
    } else {
      /*
       ? 4.设置GPIO5_IO3输出高电平
        - set GPIO5_DR to configure GPIO5_IO3 output 1
        - GPIO5_DR 0x020ac000 + 0x00
        - bit[3] = 0b1
       */
      *GPIO5_DR |= (1 << 3);
    }
  }

  return 0;
}

static char board_led_getLevel(int which) {
  char level = 0;
  int val;

  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (which == 0) {
    /*
     ? 6.获取GPIO5_IO3的GPIO5_PSR寄存器Pin的高低电平
      - get GPIO5_PSR
      - GPIO5_PSR 0x020ac000 + 0x08
      - bit[3] = x
     */
    val = *GPIO5_PSR;
    printk("GPIO5_PSR = %d\n", val);
    level = ((*GPIO5_PSR) & (1 << 3)) ? 1 : 0;
  }
  return level;
}

static int board_led_unmap(int which) {

  printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  if (which == 0) {
    iounmap(CCM_CCGR1);
    iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
    iounmap(GPIO5_GDIR);
    iounmap(GPIO5_DR);
    iounmap(GPIO5_PSR);
    //- 避免指针悬垂引用
    CCM_CCGR1 = NULL;
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = NULL;
    GPIO5_GDIR = NULL;
    GPIO5_DR = NULL;
    GPIO5_PSR = NULL;
  }
  return 0;
}

static struct led_operations board_led_opr = {
    .num = 1,
    .init = board_led_init,
    .ctl = board_led_ctl,
    .getLevel = board_led_getLevel,
    .unmap = board_led_unmap,
};

struct led_operations *get_board_led_opr(void) {
  return &board_led_opr;
}