// todo spi总线(搭配设备树)DAC模块设备驱动模板
// 通过观察LED的亮度来验证DAC的工作
/*  头文件-------------------------------------------*/
#include "asm-generic/errno-base.h"
#include "asm/gpio.h"
#include "asm/uaccess.h"
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
#include "linux/mmc/core.h"
#include "linux/mod_devicetable.h"
#include "linux/module.h"
#include "linux/of.h"
#include "linux/of_gpio.h"
#include "linux/platform_device.h"
#include "linux/poll.h"
#include "linux/printk.h"
#include "linux/slab.h"
#include "linux/spi/spi.h"
#include "linux/stddef.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*  自定义宏-----------------------------------------*/
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DAC: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

/*  变量---------------------------------------------*/
static struct spi_device *g_s_p_client;
static int g_major;
static struct class *g_s_p_class;
static DECLARE_WAIT_QUEUE_HEAD(g_s_p_wait);
static struct fasync_struct *g_s_p_fasync;

/*  自定义函数---------------------------------------*/
/*  中断处理函数-------------------------------------*/
// static void timer_expire(unsigned long data) {}
// static irqreturn_t top_isr(int fd, void *data) { return IRQ_HANDLED; }

/*  file_operations接口函数--------------------------*/
static ssize_t spi_drv_read(struct file *fp, char __user *buf, size_t size,
                            loff_t *offset) {

  /*初始化spi_transfer*/

  /* spi_sync_transfer(struct spi_device * spi, struct spi_transfer * xfers,
                       unsigned int num_xfers) */

  /*copy_to_user*/

  return 0;
}
static ssize_t spi_drv_write(struct file *fp, const char __user *buf,
                             size_t size, loff_t *offset) {
  int err;
  short val;
  struct spi_transfer t;
  u8 ker_buf[2];

  memset(&t, 0, sizeof(t));

  if (size != 2) {
    return -EINVAL;
  }

  /*copy_from_user*/
  err = copy_from_user(&val, buf, size);
  /*取中间10bit数据，DAC数据格式:高4位、低2位为0*/
  val <<= 2;
  val &= 0x0FFF;

  ker_buf[0] = (val >> 8) & 0xFF; /*高8位*/
  ker_buf[1] = val & 0xFF;        /*低8位*/

  /*初始化spi_transfer*/
  t.tx_buf = ker_buf;
  t.len = 2;

  err = spi_sync_transfer(g_s_p_client, &t, 1);
  /* spi_sync_transfer(struct spi_device * spi, struct spi_transfer * xfers,
                       unsigned int num_xfers) */

  return size;
}
static unsigned int spi_drv_poll(struct file *fp,
                                 struct poll_table_struct *table) {
  return POLLIN | POLLRDNORM;
}

static int spi_drv_fasync(int fd, struct file *fp, int mode) {
  if (fasync_helper(fd, fp, mode, &g_s_p_fasync) >= 0) {
    return 0;
  } else {
    return -EIO;
  }
}

/*  file_operations定义------------------------------*/
static struct file_operations g_s_fops = {
    .owner = THIS_MODULE,
    .poll = spi_drv_poll,
    .fasync = spi_drv_fasync,
    .read = spi_drv_read,
    .write = spi_drv_write,
};

/*  spi_driver接口函数-------------------------------*/

static int spi_drv_probe(struct spi_device *spi) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*记录client*/
  g_s_p_client = spi;

  /*注册字符设备*/
  g_major = register_chrdev(0, "template_i2c_chrdev", &g_s_fops);
  g_s_p_class = class_create(THIS_MODULE, "template_class");
  if (IS_ERR(g_s_p_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_major, "template_spi_chrdev");
    return PTR_ERR(g_s_p_class);
  }
  device_create(g_s_p_class, NULL, MKDEV(g_major, 0), NULL, "DAC"); /*/dev/DAC*/

  return 0;
}

static int spi_drv_remove(struct spi_device *spi) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*反注册字符设备*/
  device_destroy(g_s_p_class, MKDEV(g_major, 0));
  class_destroy(g_s_p_class);
  unregister_chrdev(g_major, "template_spi_chrdev");

  return 0;
}

/*  spi_driver定义---------------------------------*/
static const struct of_device_id g_s_dt_ids[] = {
    {

        .compatible = "ti,tlc5615c,tlc5615i",
    },
    {},
};
static struct spi_driver g_s_spi_drv = {
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "template_spi_drv",
            .of_match_table = g_s_dt_ids,

        },
    .probe = spi_drv_probe,
    .remove = spi_drv_remove,
};

/*  入口函数-----------------------------------------*/
static int __init dev_drv_init(void) {
  /*注册spi_driver*/
  return spi_register_driver(&g_s_spi_drv);
}

/*  出口函数-----------------------------------------*/
static void __exit dev_drv_exit(void) {
  /*卸载spi_driver*/
  spi_unregister_driver(&g_s_spi_drv);
}

/*  注册入口出口-------------------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");
