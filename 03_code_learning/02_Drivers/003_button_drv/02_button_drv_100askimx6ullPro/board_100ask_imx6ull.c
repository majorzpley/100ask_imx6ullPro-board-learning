#include "asm/io.h"
#include "button_drv.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

// todo 按键引脚为GPIO5_IO01对应KEY1、GPIO4_IO14对应KEY2

struct IMX6ULL_GPIO {
  volatile uint32_t DR;
  volatile uint32_t GDIR;
  volatile uint32_t PSR;
  volatile uint32_t ICR1;
  volatile uint32_t ICR2;
  volatile uint32_t IMR;
  volatile uint32_t ISR;
  volatile uint32_t EDGE_SEL;
};

/*
 *enable GPIO4
 */
static volatile uint32_t *CCM_CCGR3;

/*
 *enable GPIO5
 */
static volatile uint32_t *CCM_CCGR1;

/*
 * set GPIO4_IO14 as GPIO
 */
static volatile uint32_t *IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B;

/*
 * set GPIO5_IO01 as GPIO
 */
static volatile uint32_t *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1;

static struct IMX6ULL_GPIO *gpio4;
static struct IMX6ULL_GPIO *gpio5;

static void board_xxx_button_init_gpio(int which) {
  printk("%s %s %d, init gpio for button %d\n", __FILE__, __FUNCTION__,
         __LINE__, which);
  if (!CCM_CCGR1) {
    CCM_CCGR1 = ioremap(0x20c406c, 4);

    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = ioremap(0x229000c, 4);

    gpio5 = ioremap(0x20ac000, sizeof(struct IMX6ULL_GPIO));
  }
  if (!CCM_CCGR3) {
    CCM_CCGR3 = ioremap(0x20c4074, 4);

    IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B = ioremap(0x20e01b0, 4);

    gpio4 = ioremap(0x20a8000, sizeof(struct IMX6ULL_GPIO));
  }

  if (which == 0) {
    /*
     * 1.enable GPIO4
     * CG6, b[13:12] = 0b11
     */
    *CCM_CCGR3 |= (3 << 12);

    /*
     * 2.set GPIO4_IO14 as GPIO
     * MUX_MODE, b[3:0] = 0b0101
     */
    *IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B &= ~0x0f;
    *IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B |= 5;

    /*
     * 3. set GPIO4_IO14 as input
     * GPIO4 GDIR, b[14] = 0b0
     */
    gpio4->GDIR &= ~(1 << 14);
  } else if (which == 1) {
    /*
     * 1.enable GPIO5
     * CG15, b[31:30] = 0b11
     */
    *CCM_CCGR1 |= (3 << 30);

    /*
     * 2.set GPIO5_IO01 as GPIO
     * MUX_MODE, b[3:0] = 0b0101
     */
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 &= ~0x0f;
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 |= 5;

    /*
     * 3. set GPIO5_IO01 as input
     * GPIO5 GDIR, b[1] = 0b0
     */
    gpio5->GDIR &= ~(1 << 1);
  }
}

static int board_xxx_button_read_gpio(int which) {
  printk("%s %s %d, init gpio for button %d\n", __FILE__, __FUNCTION__,
         __LINE__, which);
  if (which == 0) {
    return (gpio4->PSR & (1 << 14) ? 1 : 0);
  } else if (which == 1) {
    return (gpio5->PSR & (1 << 1) ? 1 : 0);
  } else {
    return -1;
  }
}

static void board_xxx_button_unmap_gpio(int which) {
  if (which == 0) {
    iounmap(CCM_CCGR3);
    iounmap(IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B);
    iounmap(gpio4);
    CCM_CCGR3 = NULL;
    IOMUXC_SW_MUX_CTL_PAD_NAND_CE1_B = NULL;
    gpio4 = NULL;
  } else if (which == 1) {
    iounmap(CCM_CCGR1);
    iounmap(IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1);
    iounmap(gpio5);
    CCM_CCGR1 = NULL;
    IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER1 = NULL;
    gpio5 = NULL;
  }
}

static struct button_operations my_button_ops = {
    .count = 2,
    .init = board_xxx_button_init_gpio,
    .read = board_xxx_button_read_gpio,
    .unmap = board_xxx_button_unmap_gpio,
};

int __init board_xxx_button_init(void) {
  register_button_operations(&my_button_ops);
  return 0;
}

void __exit board_xxx_button_exit(void) { unregister_button_operations(); }

module_init(board_xxx_button_init);
module_exit(board_xxx_button_exit);
MODULE_LICENSE("GPL");