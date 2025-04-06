// todo i2c总线(搭配设备树)设备驱动模板
/*  头文件-------------------------------------------*/
#include "asm-generic/errno-base.h"
#include "asm/gpio.h"
#include "linux/device.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/fs.h"
#include "linux/gfp.h"
#include "linux/i2c.h"
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
#include "linux/stddef.h"
#include "linux/timer.h"
#include "linux/wait.h"

/*  自定义宏-----------------------------------------*/
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINTK(fmt, args...) printk(KERN_DEBUG "DEV: " fmt, ##args)
#else
#define DEBUG_PRINTK(fmt, args...) /*不做任何事*/
#endif

/*  变量---------------------------------------------*/
static struct i2c_client *g_s_p_client;
static int g_major;
static struct class *g_s_p_class;
static int g_count;
static DECLARE_WAIT_QUEUE_HEAD(g_s_p_wait);
static struct fasync_struct *g_s_p_fasync;

/*  自定义函数---------------------------------------*/
/*  中断处理函数-------------------------------------*/
static void timer_expire(unsigned long data) {}
static irqreturn_t top_isr(int fd, void *data) { return IRQ_HANDLED; }

/*  file_operations接口函数--------------------------*/
static ssize_t i2c_drv_read(struct file *fp, char __user *buf, size_t size,
                            loff_t *offset) {
  int err;
  struct i2c_msg msgs[2];

  /*初始化i2c_msg*/
  err = i2c_transfer(g_s_p_client->adapter, msgs, ARRAY_SIZE(msgs));

  /*copy_to_user*/

  return 0;
}
static ssize_t i2c_drv_write(struct file *fp, const char __user *buf,
                             size_t size, loff_t *offset) {
  int err;
  struct i2c_msg msgs[2];
  /*copy_from_user*/

  /*初始化i2c_msg*/
  err = i2c_transfer(g_s_p_client->adapter, msgs, ARRAY_SIZE(msgs));

  return 0;
}
static unsigned int i2c_drv_poll(struct file *fp,
                                 struct poll_table_struct *table) {
  return POLLIN | POLLRDNORM;
}

static int i2c_drv_fasync(int fd, struct file *fp, int mode) {
  if (fasync_helper(fd, fp, mode, &g_s_p_fasync) >= 0) {
    return 0;
  } else {
    return -EIO;
  }
}

/*  file_operations定义------------------------------*/
static struct file_operations g_s_fops = {
    .owner = THIS_MODULE,
    .poll = i2c_drv_poll,
    .fasync = i2c_drv_fasync,
    .read = i2c_drv_read,
    .write = i2c_drv_write,
};

/*  i2c_driver接口函数-------------------------------*/

static int i2c_drv_probe(struct i2c_client *client,
                         const struct i2c_device_id *id) {
  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  struct i2c_adapter *adapter = client->adapter;
  struct device_node *np = client->dev.of_node; /*获得设备树节点信息*/

  /*记录client*/
  g_s_p_client = client;

  /*注册字符设备*/
  g_major = register_chrdev(0, "template_i2c_chrdev", &g_s_fops);
  g_s_p_class = class_create(THIS_MODULE, "template_class");
  if (IS_ERR(g_s_p_class)) {
    DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    unregister_chrdev(g_major, "template_i2c_chrdev");
    return PTR_ERR(g_s_p_class);
  }
  device_create(g_s_p_class, NULL, MKDEV(g_major, 0), NULL,
                "template_i2c"); /*/dev/teplate_i2c*/

  return 0;
}

static int i2c_drv_remove(struct i2c_client *client) {

  DEBUG_PRINTK("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

  /*反注册字符设备*/
  device_destroy(g_s_p_class, MKDEV(g_major, 0));
  class_destroy(g_s_p_class);
  unregister_chrdev(g_major, "template_i2c_chrdev");

  return 0;
}

/*  i2c_driver定义---------------------------------*/
static const struct of_device_id g_s_dt_ids[] = {
    {
        .compatible = "100ask,i2c_dev",
    },
    {},
};
static struct i2c_driver g_s_i2c_drv = {
    .driver =
        {
            .name = "template_i2c_drv",
            .of_match_table = g_s_dt_ids,

        },
    .probe = i2c_drv_probe,
    .remove = i2c_drv_remove,
};

/*  入口函数-----------------------------------------*/
static int __init dev_drv_init(void) {
  /*注册i2c_driver*/
  return i2c_add_driver(&g_s_i2c_drv);
}

/*  出口函数-----------------------------------------*/
static void __exit dev_drv_exit(void) {
  /*卸载i2c_driver*/
  i2c_del_driver(&g_s_i2c_drv);
}

/*  注册入口出口-------------------------------------*/
module_init(dev_drv_init);
module_exit(dev_drv_exit);
MODULE_LICENSE("GPL");
