//? 最简单的LED驱动程序-IMX6ULLPRO
#include <asm/io.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uaccess.h>

static int major;
struct class *led_class;

//? registers
//-复用功能IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3地址:0x02290000h-base+0x14h-offset
static volatile unsigned int *IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3;

//-GPIO5_GDIR地址:0x20ac004
static volatile unsigned int *GPIO5_GDIR;

//-GPIO5_DR地址:0x20ac000
static volatile unsigned int *GPIO5_DR;

static int led_open(struct inode *inode, struct file *f) {
  // todo configure
  /*
   * enable gpio5:默认使能
   *configure gpio5_io3 as gpio
   *configure pin as output
   */
  *IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 &= ~0xf;
  *IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 |= 0x05; // iomux

  *GPIO5_GDIR |= 1 << 3; // output

  return 0;
}
static ssize_t led_write(struct file *file, const char __user *buf, size_t len,
                         loff_t *offset) {
  char val;
  int ret;
  // todo copy_rom_user: get data from app
  ret = copy_from_user(&val, buf, 1);
  // todo to set gpio register: out 1/0
  if (val == 1) {
    //- set gpio to let led on
    *GPIO5_DR &= ~(1 << 3);
  } else {
    //- set gpio to let led off
    *GPIO5_DR |= 1 << 3;
  }
  return len;
}

static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open = led_open,
};

//- 入口函数
static int __init led_init(void) {
  printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  major = register_chrdev(0, "100ask_led", &led_fops);

  // todo ioremap映射GPIO地址
  //-复用功能IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3地址:0x02290000h-base+0x14h-offset
  IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = ioremap(0x02290000 + 0x14, 4);

  //-GPIO5_GDIR地址:0x20ac004
  GPIO5_GDIR = ioremap(0x020ac004, 4);

  //-GPIO5_DR地址:0x20ac000
  GPIO5_DR = ioremap(0x020ac000, 4);

  led_class = class_create(THIS_MODULE, "myled");
  device_create(led_class, NULL, MKDEV(major, 0), NULL,
                "myled"); //- /dev/myled

  return 0;
}
//- 出口函数
static void __exit led_exit(void) {
  // todo 接触映射
  iounmap(IOMUX_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3);
  iounmap(GPIO5_GDIR);
  iounmap(GPIO5_DR);
  device_destroy(led_class, MKDEV(major, 0));
  class_destroy(led_class);
  unregister_chrdev(major, "100ask_led");
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");